#ifndef DBMANAGER_H
#define DBMANAGER_H

/*		System Includes		*/

/*		QT Includes			*/
#include <QtSql/QSqlDatabase>

/*		Stellar Includes	*/

/*		App Includes		*/
#include "DataStructures.h"

class DBManager
{
private:
    QSqlDatabase Manager;
    bool DbOpen;

	
	
public:
    DBManager();
    ~DBManager();

    void MakeConnection();
    void InsertIntoPersonalInfo(QString, QString, QString);
	void InsertIntoPasswords(QString, QString);
	void InsertCustomerID(quint32);
	void StoreStellarInfo(QString, QString, QString, QString);
	
	bool CheckInfo(QString email, QString pswrd);
	bool CheckUserExist(QString email);

	quint32 GetLatestCustID();
	QString GetCustomerID(QString email);
	QString GetName(QString email);

	QString GetAccountID(QString CustomerID);
	QString GetPublicKey(QString CustomerID);
	QString GetSecretKey(QString CustomerID);

	StellarInfo GetStellarInfo(QString AccountID);

	void SaveLatestToken(QString token);
	QString LoadLatestToken();
	QString GetAccName(QString AccID);

	void DeleteAllData();
};

#endif // DBMANAGER_H
