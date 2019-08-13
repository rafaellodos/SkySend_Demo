#pragma once
/*		System Includes		*/

/*		QT Includes			*/
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>


/*		Stellar Includes	*/
#include "src/KeyPair.h"
#include "src/Server.h"
#include "src/responses/accountresponse.h"

/*		App Includes		*/
#include "dbManager.h"
#include "DataStructures.h"

class SkySendAccount : public QObject
{
	Q_OBJECT

private:

	//Members
	//Connecting to stellar network
	Server* mServer;
	DBManager* pDB;
	KeyPair* mAccount;
	QVector<Balances> mBalances;
	quint64 curSequenceNumber;

	//Methods
	void SendLumens(QString destAccID, QString amount, bool emitSignal);
	void SendCustomAsset(QString destAccID, QString amount, QString asset, bool Addtrust, bool emitSignal);

	//methods 
	void WorkoutSequenceNumber(quint64 newSequenceNumber);

public:
	SkySendAccount();
	~SkySendAccount();
		
	//Getters & Setters
	//Getters
	KeyPair* GetCurAccKeypair() { return mAccount; }

	//Setters
	void SetDBmanager(DBManager* inDB) { pDB = inDB; }
	void SetCurAccKeypair(QString inSecretSeed) { mAccount = KeyPair::fromSecretSeed(inSecretSeed); }
	

	void InitBalances();	
	QString GetBalance();
	QVector<Balances> GetAllBalances() { return mBalances; }
	
	void SendMoney(QString destAccID, QString amount, QString SenderAsset, QString RecipientAsset, bool emitSignal = true);
	void ManagerOffer(QString SellAsset, QString BuyAsset, QString Amount, QString Price, bool emitSignal = true);

	void CheckTrust(QString AccountID, QString assetTrusted);

	signals:
	void accountGood();
	void accountUpdate();
	void accountSuccess();
	void assetTrust(bool);
	void makeExchanges();
	void OfferSubmitted();
	void MoneySent();
};

