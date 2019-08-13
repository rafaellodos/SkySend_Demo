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
#include "SkySendAccount.h"

//DEFINES
#define SOURCE_ACC_ID ("GDDFZ6AHEWCENMVQEFH46TCAEUP6I3ISWM7NBYNJUPYILACJJIUCLY4H")
#define ISSUER_ACC_ID ("GBQFEG4IOJZ6BKVZ6XOT7H3P5G4SMTUWCUMOMGF25JMJU73PKEB4P6DF")

SkySendAccount::SkySendAccount() : curSequenceNumber(0)
{
	Network::useTestNetwork();//select the network to use
	mServer = new Server("https://horizon-testnet.stellar.org");//choose a horizon host
}
SkySendAccount::~SkySendAccount()
{
}

void SkySendAccount::InitBalances()
{
	AccountResponse * response = mServer->accounts().account(mAccount);

	QObject::connect(response, &AccountResponse::ready, [response, this]() {
		int index = 0;
		for (auto balance : response->getBalances())
		{
			Balances curBalance;
			curBalance.index = index;
			curBalance.BalanceAmount = balance.getBalance();
			curBalance.AssetType = balance.getAssetType();
			curBalance.AssetCode = balance.getAssetCode();
			
			if (balance.getAssetType() != "native")
				curBalance.AssetIssuer = balance.getAssetIssuer().getAccountId();
			else if (balance.getAssetType() == "native")
				curBalance.AssetCode = "Lumens";

			mBalances << curBalance;
			index++;
		}
		emit accountGood();
	});
}
QString SkySendAccount::GetBalance()
{
	AccountResponse * response = mServer->accounts().account(mAccount);
	QObject::connect(response, &AccountResponse::ready, [response, this]() {
		int i = 0;
		for (auto balance : response->getBalances())
		{
			mBalances[i].BalanceAmount = balance.getBalance();
			i++;
		}
		emit accountUpdate();
	});

	return "Loading...";
}


void SkySendAccount::SendMoney(QString destAccID, QString amount, QString SenderAsset, QString RecipientAsset, bool emitSignal)
{
	if (SenderAsset == RecipientAsset)
	{
		if (SenderAsset == "Lumens")
		{
			SendLumens(destAccID, amount, emitSignal);
		}
		else
		{
			QObject::connect(this, &SkySendAccount::assetTrust, [this, destAccID, amount, SenderAsset, emitSignal](bool accountGood)
			{
				SendCustomAsset(destAccID, amount, SenderAsset, accountGood, emitSignal);
			});
			CheckTrust(destAccID, SenderAsset);
		}
	}
	else
	{
		//EMIT signal to deal with this elsewhere. It makes more sense to multiple instances of a skysend account, 
		//which then can make offers and use the existing functions to make it work
		emit makeExchanges();
	}
}
void SkySendAccount::SendLumens(QString destAccID, QString amount, bool emitSignal)
{
	AccountResponse* AccResponse = mServer->accounts().account(mAccount);
	QObject::connect(AccResponse, &AccountResponse::ready, this, [AccResponse, destAccID, amount, emitSignal, this]() 
	{
		WorkoutSequenceNumber(AccResponse->getSequenceNumber());
		Account* account = new Account(mAccount, curSequenceNumber);
		KeyPair* destination = KeyPair::fromSecretSeed(destAccID);

		// Start building the transaction
		Transaction::Builder *builder = new Transaction::Builder(account);
		builder->addOperation(new PaymentOperation(destination, new AssetTypeNative(), amount));
		Transaction * mTrans = builder->build();

		// Sign the transaction to prove you are actually the person sending it.
		mTrans->sign(mAccount);
		mServer->submitTransaction(mTrans);

		if (emitSignal)
		{
			emit accountSuccess();
		}
		else
		{
			emit MoneySent();
		}

	});
}
void SkySendAccount::SendCustomAsset(QString destAccID, QString amount, QString asset, bool addTrust, bool emitSignal)
{
	AccountResponse* AccResponse;
	
	KeyPair* destination = KeyPair::fromSecretSeed(destAccID);
	//create asset representation
	Asset* sentAsset = Asset::createNonNativeAsset(asset, KeyPair::fromAccountId(ISSUER_ACC_ID));
	
	if (!addTrust)
	{
		//Add trustline to Asset
		AccResponse = mServer->accounts().account(destination);
		QObject::connect(AccResponse, &AccountResponse::ready, this, [this, AccResponse, destination, amount, sentAsset]()
		{
			qint64 sequenceNumber = AccResponse->getSequenceNumber();
			Account* account = new Account(destination, sequenceNumber);

			// Start building the transaction.
			Transaction::Builder *builder = new Transaction::Builder(account);

			Operation* createTrust = new ChangeTrustOperation(sentAsset, "1000");
			builder->addOperation(createTrust);

			Transaction * mTrans = builder->build();
			// Sign the transaction to prove you are actually the person sending it.
			mTrans->sign(destination);
			mServer->submitTransaction(mTrans);

		});
	}
	
	AccResponse = mServer->accounts().account(mAccount);
	QObject::connect(AccResponse, &AccountResponse::ready, this, [this, AccResponse, destination, amount, emitSignal, sentAsset]()
	{
		WorkoutSequenceNumber(AccResponse->getSequenceNumber());
		Account* account = new Account(mAccount, curSequenceNumber);

		// Start building the transaction.
		Transaction::Builder *builder = new Transaction::Builder(account);
		builder->addOperation(new PaymentOperation(destination, sentAsset, amount));
		Transaction * mTrans = builder->build();

		// Sign the transaction to prove you are actually the person sending it.
		mTrans->sign(mAccount);
		mServer->submitTransaction(mTrans);

		if(emitSignal)
			emit accountSuccess();

	});
}

void SkySendAccount::WorkoutSequenceNumber(quint64 newSequenceNumber)
{
	if (curSequenceNumber != newSequenceNumber)
	{
		curSequenceNumber = newSequenceNumber;
	}
	else
	{
		curSequenceNumber++;
	}
}

void SkySendAccount::ManagerOffer(QString SellAsset, QString BuyAsset, QString Amount, QString Price, bool emitSignal)
{
	AccountResponse* AccResponse = mServer->accounts().account(mAccount);

	//create asset representation
	Asset* sellingAsset;
	Asset* buyingAsset;
	//create selling asset
	if (SellAsset == "Lumens")
		sellingAsset = new AssetTypeNative();
	else
		sellingAsset = Asset::createNonNativeAsset(SellAsset, KeyPair::fromAccountId(ISSUER_ACC_ID));

	if (BuyAsset == "Lumens")
		buyingAsset = new AssetTypeNative();
	else
		buyingAsset = Asset::createNonNativeAsset(BuyAsset, KeyPair::fromAccountId(ISSUER_ACC_ID));
	
	
	QObject::connect(AccResponse, &AccountResponse::ready, this, [this, AccResponse, sellingAsset, buyingAsset, Amount, Price, emitSignal]()
	{
		WorkoutSequenceNumber(AccResponse->getSequenceNumber());
		Account* account = new Account(mAccount, curSequenceNumber);

		// Start building the transaction.
		Transaction::Builder *builder = new Transaction::Builder(account); 
		ManageOfferOperation* newOp = new ManageOfferOperation(sellingAsset, buyingAsset, Amount, Price, 0);
		builder->addOperation(newOp);
		// Sign the transaction to prove you are actually the person sending it.
		Transaction * mTrans = builder->build();
		mTrans->sign(mAccount);
		//TransactionResponse *mResponse = mServer->transactions().transaction();
		mServer->submitTransaction(mTrans);

		delete sellingAsset;
		delete buyingAsset;
		if(emitSignal)
			emit OfferSubmitted();
	});
}

void SkySendAccount::CheckTrust(QString AccountID, QString assetTrusted)
{
	KeyPair* AccToCheck = KeyPair::fromSecretSeed(AccountID);
	AccountResponse * response = mServer->accounts().account(AccToCheck);
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
