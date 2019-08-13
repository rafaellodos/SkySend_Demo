#pragma once

/*		System Includes		*/

/*		QT Includes			*/
#include <QtWidgets/QMainWindow>
#include <QtWidgets/qpushbutton.h>
#include "ui_SkySend_DemoVS.h"

/*		Stellar Includes	*/

/*		App Includes		*/
#include "dbManager.h"
#include "AccountManager.h"
#include "SkySendAccount.h"



class SkySend_DemoVS : public QMainWindow
{
	Q_OBJECT

public:
	SkySend_DemoVS(QWidget *parent = Q_NULLPTR);

private:
	//Members
	Ui::SkySend_DemoVSClass ui;
	DBManager mDB;
	
	//Used to create account
	AccountManager mAccountManager;
	
	//Skysend Account
	SkySendAccount mUserAccount;
	
	QString curUserEmail;
	QString Password;
	QString confPassword;

	QVector<QFrame*> allFrames;
    //Methods
	
	//Flow Control
	bool BalanceSetupDone;
	void SetFrameVisible(QFrame* inFrame);

	//Initializations
	void InitSetup();
	void InitLoginSetup(QString email);

	void CreatingAccount(QString email, QString name = "");

	void MakeExchanges(QString destSecretKey, QString Amount, QString SellCurrency, QString BuyCurrency);


	//Validation
	bool ValidateName(QString inName);
	bool ValidateEmail(QString inEmail, bool EmailExists = false);
	bool ValidatePassword(QString inPass);

public slots:
	void CreateAccount();
	void LoginIntoAccount();
	void SetUpHomeScreen(QString currUser);
	void SendMoney();

	//Create Account events
	void CA_ok_clicked();
	void CA_cancel_clicked();
	void CA_PasswordEncryption(QString);
	void CA_ConfPasswordEncryption(QString);

	//Login Events
	void Login_ok_clicked();
	void Login_cancel_clicked();
	void Login_PasswordEncryption(QString);

	//Home Screen Events
	void Home_ChangeCurrency(int index);
	void Home_SendMoney_clicked();
	void Home_UpdateBalance();	
	void UpdateCurrencyAndBalance();
	void Home_MakeOffer_clicked();

	void SM_ok_clicked();
	void SM_cancel_clicked();

	void MF_MakeOffer_clicked();
	void MF_Cancel_clicked();

};
