/*		System Includes		*/
#include <dos.h>

/*		QT Includes			*/
#include <QtWidgets/qmessagebox>

/*		Stellar Includes	*/
#include "src/Network.h"
#include "src/Account.h"
#include "src/transaction.h"
#include "src/paymentoperation.h"
#include "src/responses/operations/paymentoperationresponse.h"
#include "src/assettypecreditalphanum.h"
#include "src/ChangeTrustOperation.h"
#include "src/ManageOfferOperation.h"
#include "src/CreateAccountOperation.h"

/*		App Includes		*/
#include "AccountManager.h"

//DEFINES
#define SOURCE_ACC_ID ("GDDFZ6AHEWCENMVQEFH46TCAEUP6I3ISWM7NBYNJUPYILACJJIUCLY4H")
#define ISSUER_ACC_ID ("GBQFEG4IOJZ6BKVZ6XOT7H3P5G4SMTUWCUMOMGF25JMJU73PKEB4P6DF")

AccountManager::AccountManager()
{
	Network::useTestNetwork();//select the network to use
	server = new Server("https://horizon-testnet.stellar.org");//choose a horizon host
}
AccountManager::~AccountManager()
{
}

bool AccountManager::CreateTestAccount(QString CustID)
{
	KeyPair* newAccountAdd = KeyPair::random(QByteArray(CustID.toUtf8()));
	pDB->StoreStellarInfo(CustID, newAccountAdd->getAccountId(), newAccountAdd->getPublicKey(), newAccountAdd->getSecretSeed());
	
	QString url = "https://horizon-testnet.stellar.org/friendbot?addr=";

	RequestAccountCreation(url + newAccountAdd->getAccountId());

	return false;
}
void AccountManager::RequestAccountCreation(QString url)
{
	NetworkManager = new QNetworkAccessManager();
	QObject::connect(NetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
	QUrl qrl(url);
	QNetworkRequest request;
	request.setUrl(qrl);
	NetworkManager->get(QNetworkRequest(qrl));

}
void AccountManager::replyFinished(QNetworkReply* reply)
{
	QByteArray bytes = reply->readAll();
	QString str = QString::fromUtf8(bytes.data(), bytes.size());
	int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (statusCode == 302 || statusCode == 307)
	{
		QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

		QNetworkRequest newRequest(newUrl);
		NetworkManager->get(newRequest);
		return;
	}
	else if (statusCode == 200)
	{
		QMessageBox Msgbox;
		Msgbox.setText("Success creating account!!!!!");
		Msgbox.exec();

		emit accountSuccess();
	}
	else
	{
		QString Error;
		
		Error =  "Error\nhttp code " + QVariant(statusCode).toString();

		if (str.contains("op_already_exist"))
		{
			Error += "\nAccount already exist";
			QMessageBox Msgbox;
			Msgbox.setText(Error);
			Msgbox.exec();
			emit accountSuccess();
		}
		else
		{
			Error += "\n" + str;	
			QMessageBox Msgbox;
			Msgbox.setText(Error);
			Msgbox.exec();
			emit accountFail();
		}
	}
};
bool AccountManager::DeleteAccount()
{
	return false;
}

void AccountManager::CreateAccount(QString CustID)
{
	KeyPair* newAccountAdd = KeyPair::random(QByteArray(CustID.toUtf8()));
	pDB->StoreStellarInfo(CustID, newAccountAdd->getAccountId(), newAccountAdd->getPublicKey(), newAccountAdd->getSecretSeed());

	KeyPair* sourceAccAdd = getSourceAcc();

	AccountResponse* mAccountResponse = server->accounts().account(sourceAccAdd);
	QObject::connect(mAccountResponse, &AccountResponse::ready, [sourceAccAdd, mAccountResponse, newAccountAdd, this]()
	{
		Account* sourceAccount = new Account(sourceAccAdd, mAccountResponse->getSequenceNumber());

		Transaction::Builder *builder = new Transaction::Builder(sourceAccount);

		Operation* createOp = new CreateAccountOperation(newAccountAdd, "10.00");
		builder->addOperation(createOp);

		Transaction * mTrans = builder->build();

		mTrans->sign(sourceAccAdd);
		server->submitTransaction(mTrans);

		QMessageBox Msgbox;
		Msgbox.setText("Success creating account!!!!!");
		Msgbox.exec();
		emit accountSuccess();
	});
}

void AccountManager::CheckTrust(QString AccountID, QString assetTrusted)
{
	KeyPair* AccToCheck = KeyPair::fromSecretSeed(AccountID);
	AccountResponse * response = server->accounts().account(AccToCheck);
	bool found = false;
	QObject::connect(response, &AccountResponse::ready, [response, &found, assetTrusted, this]() {
		for (auto balance : response->getBalances())
		{
			if (assetTrusted == balance.getAssetCode())
				found = true;
		}
		emit assetTrust(found);
	});
}


KeyPair * AccountManager::getSourceAcc()
{
	return KeyPair::fromSecretSeed(pDB->GetStellarInfo(SOURCE_ACC_ID).SecretKey);
}

void AccountManager::PaymentWatcher(QString sourceSecretKey)
{
	KeyPair* accountAdd = KeyPair::fromSecretSeed(sourceSecretKey);
	// Create an API call to query payments involving the account.
	PaymentsRequestBuilder paymentsRequest = server->payments().forAccount(accountAdd).order(RequestBuilder::Order::ASC);
	paymentsRequest.stream();

	// If some payments have already been handled, start the results from the
	// last seen payment. (See below in `handlePayment` where it gets saved.)
	QString lastToken = pDB->LoadLatestToken();
	if (lastToken != NULL)
		paymentsRequest.cursor(lastToken);

	// `stream` will send each recorded payment, one by one, then keep the
	// connection open and continue to send you new payments as they occur.
	OperationPage * response = paymentsRequest.execute();

	QObject::connect(response, &OperationResponse::ready, this, [response, this]()
	{
		if (response->size() == 0)
			return;
		auto r = response->streamedElement();
		QString token = ((OperationResponse*)r)->getPagingToken();
		pDB->SaveLatestToken(token);
		OperationResponse* payment = r;

		if (payment->getType() == "payment")
		{
			QString amount = ((PaymentOperationResponse*)payment)->getAmount();

			Asset *asset = ((PaymentOperationResponse*)payment)->getAsset();
			QString assetName;
			if (asset->equals(new AssetTypeNative())) 
			{
				assetName = "lumens";
			}
			else 
			{

				assetName.append(((AssetTypeCreditAlphaNum*)asset)->getCode());
				assetName.append(":");
				assetName.append(((AssetTypeCreditAlphaNum*)asset)->getIssuer().getAccountId());

			}

			QString output;
			output.append(amount);
			output.append(" ");
			output.append(assetName);
			output.append(" from ");
			output.append(pDB->GetAccName(((PaymentOperationResponse*)payment)->getFrom().getAccountId()));
			output.append(" to ");
			output.append(pDB->GetAccName(((PaymentOperationResponse*)payment)->getTo().getAccountId()));

			//TODO - Some sort of notification.
			qDebug() << output;
		}
	});
}
