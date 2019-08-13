/*		System Includes		*/

/*		QT Includes			*/
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QDebug>

/*		Stellar Includes	*/

/*		App Includes		*/
#include "dbManager.h"

void DBManager::DeleteAllData()
{
	if (DbOpen)
	{
		QSqlQuery query;
		query.exec("DELETE FROM StellarInfo");
		query.exec("DELETE FROM CustomerID");
		query.exec("DELETE FROM Passwords");
		query.exec("DELETE FROM PersonalInfo");
		query.exec("DELETE FROM TokenTable");
	}
}

DBManager::DBManager()
{
    
}
DBManager::~DBManager()
{
    if(DbOpen)
    {
        Manager.close();
    }
}
void DBManager::MakeConnection()
{
	auto driveCheck = QSqlDatabase::isDriverAvailable("QODBC");
    if(driveCheck)
    {
        Manager = QSqlDatabase::addDatabase("QODBC");
        Manager.setDatabaseName("Driver={Microsoft Access Driver (*.mdb)};FIL={MS Access};DBQ=C:\\SkySending\\Databases\\Database1.mdb");

        DbOpen = Manager.open();
        if(!DbOpen)
            qDebug() << Manager.lastError().text();
    }
}
void DBManager::InsertIntoPersonalInfo(QString CustID, QString Name, QString Email)
{
    if(DbOpen)
    {
        QSqlQuery query;
        query.prepare("INSERT INTO PersonalInfo (CustomerID, name, EmailAddress) "
                      "VALUES (:CustomerID, :name, :EmailAddress)");
		query.bindValue(":CustomerID", CustID);
        query.bindValue(":name", Name);
        query.bindValue(":EmailAddress", Email);
        query.exec();
    }
}

void DBManager::InsertIntoPasswords(QString CustID, QString Pass)
{

	if (DbOpen)
	{
		QSqlQuery query;
		query.prepare("INSERT INTO Passwords (CustomerID, Password) "
			"VALUES (:CustomerID, :Password)");
		query.bindValue(":CustomerID", CustID);
		query.bindValue(":Password", Pass);
		query.exec();
	}
}

void DBManager::InsertCustomerID(quint32 ID)
{
	if (DbOpen)
	{
		QSqlQuery query;
		query.prepare("INSERT INTO CustomerID (ID) "
			"VALUES (:ID)");
		query.bindValue(":ID", ID);
		query.exec();
	}
}

void DBManager::StoreStellarInfo(QString CustomerID, QString AccountID, QString PublicKey, QString SecretKey)
{
	if (DbOpen)
	{
		QSqlQuery query;
		query.exec("INSERT INTO StellarInfo(CustomerID, AccountID, PublicKey, SecretKey) "
			"VALUES('"+ CustomerID +"', '"+ AccountID +"', '"+ PublicKey +"', '"+ SecretKey +"');");
		
		query.exec();
	}
}

bool DBManager::CheckInfo(QString email, QString pswrd)
{
	if (DbOpen)
	{
		QString myQuery = "SELECT Passwords.Password, PersonalInfo.EmailAddress \
			FROM Passwords INNER JOIN PersonalInfo ON Passwords.CustomerID = PersonalInfo.CustomerID \
			WHERE(((PersonalInfo.EmailAddress) = '"+ email + "'));";
		QSqlQuery query;
		query.exec(myQuery);
		QString DB_pswrd = 0;
		QString DB_email = 0;
		while (query.next())
		{
			DB_pswrd = query.value(0).toString();
			DB_email = query.value(1).toString();
		}
		
		if ((email == DB_email) && 
			(pswrd == DB_pswrd))
			return true;
	}
	
	return false;
}

bool DBManager::CheckUserExist(QString email)
{
	if (DbOpen)
	{
		QString myQuery = "SELECT EmailAddress FROM PersonalInfo WHERE EmailAddress = '" + email + "';";
		QSqlQuery query;
		query.exec(myQuery);
		QString DB_email = 0;
		while (query.next())
		{
			DB_email = query.value(0).toString();
		}

		if (email == DB_email)
			return true;
	}
	return false;
}

quint32 DBManager::GetLatestCustID()
{
	if (DbOpen)
	{
		QSqlQuery query("SELECT ID FROM CustomerID WHERE ID = (select max(id) from CustomerID)");
		int fieldNo = query.record().indexOf("ID");
		auto LatestID = 0;
		while (query.next()) 
		{
			LatestID = query.value(fieldNo).toInt();
		}
		return LatestID;
	}
	return 0;
}

QString DBManager::GetCustomerID(QString email)
{
	if (DbOpen)
	{
		QString myQuery = "SELECT CustomerID FROM  PersonalInfo WHERE EmailAddress = '" + email + "';";
		QSqlQuery query;
		query.exec(myQuery);
		while (query.next())
		{
			return query.value(0).toString();
		}
	}
	return "";
}

QString DBManager::GetName(QString email)
{
	if (DbOpen)
	{
		QString myQuery = "SELECT Name FROM PersonalInfo WHERE EmailAddress = '" + email + "';";
		QSqlQuery query;
		query.exec(myQuery);
		while (query.next())
		{
			return query.value(0).toString();
		}
	}
	return "";
}

QString DBManager::GetAccountID(QString CustomerID)
{
	if (DbOpen)
	{
		QString myQuery = "SELECT AccountID FROM  StellarInfo WHERE CustomerID = '" + CustomerID + "';";
		QSqlQuery query;
		query.exec(myQuery);
		while (query.next())
		{
			return query.value(0).toString();
		}
	}
	return "";
}

QString DBManager::GetPublicKey(QString CustomerID)
{
	if (DbOpen)
	{
		QString myQuery = "SELECT PublicKey FROM  StellarInfo WHERE CustomerID = '" + CustomerID + "';";
		QSqlQuery query;
		query.exec(myQuery);
		while (query.next())
		{
			return query.value(0).toString();
		}
	}

	return "";
}

QString DBManager::GetSecretKey(QString CustomerID)
{
	if (DbOpen)
	{
		QString myQuery = "SELECT SecretKey FROM StellarInfo WHERE CustomerID = '" + CustomerID + "';";
		QSqlQuery query;
		query.exec(myQuery);
		while (query.next())
		{
			return query.value(0).toString();
		}
	}

	return "";
}

StellarInfo DBManager::GetStellarInfo(QString AccountID)
{
	StellarInfo info;
	if (DbOpen)
	{
		QString myQuery = "SELECT * FROM StellarInfo WHERE AccountID = '" + AccountID + "';";
		QSqlQuery query;
		query.exec(myQuery);
		while (query.next())
		{
			info.AccID =  query.value(1).toString();
			info.PublicKey = query.value(2).toString();
			info.SecretKey = query.value(3).toString();
		}
	}
	return info;
}

void DBManager::SaveLatestToken(QString token)
{
	QString mQuery = "INSERT INTO TokenTable(Token) "
		"VALUES('" + token + "');";
	QSqlQuery query;
	query.exec(mQuery);
}

QString DBManager::LoadLatestToken()
{
	QString myQuery = "SELECT TOP 1 Token FROM TokenTable ORDER BY ID DESC;";
	QSqlQuery query;
	query.exec(myQuery);
	QString LatestID = "";
	while (query.next())
	{
		LatestID = query.value(0).toString();
	}
	return LatestID;
}

QString DBManager::GetAccName(QString AccID)
{
	QString myQuery = "SELECT Name "
		"FROM PersonalInfo INNER JOIN StellarInfo ON PersonalInfo.CustomerID = StellarInfo.CustomerID "
		"WHERE StellarInfo.AccountID = '" + AccID + "';";
	QSqlQuery query;
	query.exec(myQuery);
	QString LatestID = AccID;
	while (query.next())
	{
		LatestID = query.value(0).toString();
	}
	return LatestID;
}

