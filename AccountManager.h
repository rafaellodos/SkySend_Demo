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

class AccountManager : public QObject
{
	Q_OBJECT

private:

	//Members
	//Connecting to stellar network
	QNetworkAccessManager * NetworkManager;
	Server *server;
	DBManager* pDB;

	//Methods
	void RequestAccountCreation(QString url);
	void CheckTrust(QString AccountID, QString assetTrusted);
	
	KeyPair* getSourceAcc();

	private slots:
	void replyFinished(QNetworkReply*);

public:
	AccountManager(); 
	~AccountManager();
		
	//Setters and Getters
	void SetDBmanager(DBManager* inDB) { pDB = inDB; }
	Server* getServer() { return server; }

	//Operations
	bool CreateTestAccount(QString CustID);
	void CreateAccount(QString CustID);
	bool DeleteAccount();
	
	//Responses
	void PaymentWatcher(QString sourceSecretKey);

	signals:
	void accountSuccess();
	void accountFail();
	void assetTrust(bool);
};

