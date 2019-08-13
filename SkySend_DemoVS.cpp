/*		System Includes		*/
#include <windows.h>

/*		QT Includes			*/
#include <QtWidgets/qpushbutton>
#include <QtWidgets/qmessagebox>

/*		Stellar Includes	*/

/*		App Includes		*/
#include "SkySend_DemoVS.h"

#define WAIT_TO_RESOLVE (5000)

SkySend_DemoVS::SkySend_DemoVS(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);	

	allFrames << ui.CA_Frame;
	allFrames << ui.CAdone;
	allFrames << ui.Home_Frame;
	allFrames << ui.Init_Frame;
	allFrames << ui.Login_Frame;
	allFrames << ui.SM_Frame;
	allFrames << ui.MF_Frame;

	//setGeometry(1250, 500, 700, 600);

	InitSetup();
}

//Helper functions

//UI Stuff
void SkySend_DemoVS::SetFrameVisible(QFrame * inFrame)
{
	for (auto frame : allFrames)
	{
		frame->setVisible(false);
		if (frame == inFrame)
		{
			inFrame->setVisible(true);
		}
	}
}

//Setup
void SkySend_DemoVS::InitSetup()
{
	//Database Connection 
	mDB.MakeConnection();

	//UI SETUP
	//Set main Window
	setGeometry(1250, 500, 700, 600);
	ui.SM_CurrencyRecipient->addItem("Lumens");
	ui.SM_CurrencyRecipient->addItem("Rebucks");
	ui.SM_CurrencyRecipient->addItem("Sbucks");
	
	//Init Frame
	ui.Init_Frame->setGeometry(200, 200, 261, 251);
	ui.AccountNameLabel->setVisible(false);

	//Other Frames
	SetFrameVisible(ui.Init_Frame);
}

//Account Creation Events
void SkySend_DemoVS::CreateAccount()
{
	Password = "";

	ui.CA_InvalidName->setVisible(false);
	ui.CA_InvalidEmail->setVisible(false);
	ui.CA_InvalidPass->setVisible(false);
	
	//Create Account Frame
	ui.CA_Frame->setGeometry(30, 110, 621, 411);
	
	//Other Frames
	SetFrameVisible(ui.CA_Frame);
}
void SkySend_DemoVS::CA_PasswordEncryption(QString inString)
{
	static bool passAlreadyCap = false;
	if (!passAlreadyCap && inString.length() >= 1)
	{
		Password += inString[inString.length() - 1];
		QString boxText = "";
		boxText.resize(inString.length(), '*');
		passAlreadyCap = true;
		ui.CA_passwIn->setText(boxText);
	}
	else
		passAlreadyCap = false;
}
void SkySend_DemoVS::CA_ConfPasswordEncryption(QString inString)
{
	static bool confAlreadyCap = false;
	if (!confAlreadyCap && inString.length() >= 1)
	{
		confPassword += inString[inString.length() - 1];
		QString boxText = "";
		boxText.resize(inString.length(), '*');
	
		confAlreadyCap = true;
		ui.CA_confpassIn->setText(boxText);
	}
	else
	{
		confAlreadyCap = false;
	}
}
void SkySend_DemoVS::CA_ok_clicked()
{
	bool ValidInfo = true;	
	ui.CA_InvalidEmail->setVisible(false);
	ui.CA_InvalidName->setVisible(false);
	ui.CA_InvalidPass->setVisible(false);

	if (!ValidateName(ui.CA_nameIn->text()))
		ValidInfo = false;

	if (!ValidateEmail(ui.CA_emailIn->text()))
		ValidInfo = false;
	
	if (!ValidatePassword(Password))
	{
		Password = "";
		ui.CA_passwIn->setText("");
		ValidInfo = false;
	}
	
	if (Password != confPassword)
	{
		Password = "";
		confPassword = "";

		ui.CA_confpassIn->setText("");
		ui.CA_passwIn->setText("");

		QMessageBox Msgbox;
		Msgbox.setText("Passwords don't match");
		Msgbox.exec();
	}
	else if(ValidInfo)
	{
		CreatingAccount(ui.CA_emailIn->text(), ui.CA_nameIn->text() );

		ui.CAdone->setGeometry(10, 80, 671, 351);
		SetFrameVisible(ui.CAdone);
	}
}
void SkySend_DemoVS::CreatingAccount(QString email, QString name)
{
	//Get previous ID from database
	quint32 CurrID = mDB.GetLatestCustID();

	//Add 1.
	CurrID++;
	mDB.InsertCustomerID(CurrID);

	//TODO - randonize it better
	QString CustomerID = QString::number(CurrID);
	if (CustomerID.size() > 32)
		CustomerID.truncate(32);
	else if (CustomerID.size() < 32)
		CustomerID.resize(32, 'a');

	//insert personal info into database
	mDB.InsertIntoPersonalInfo(CustomerID, name, email.toLower());
	mDB.InsertIntoPasswords(CustomerID, Password);
	QObject::connect(&mAccountManager, &AccountManager::accountSuccess, this, [=]() {InitLoginSetup(email); });

	mAccountManager.SetDBmanager(&mDB);
	if (ui.CA_TestAccCheckBox->isChecked())
		mAccountManager.CreateTestAccount(CustomerID);
	else
		mAccountManager.CreateAccount(CustomerID);
}

void SkySend_DemoVS::MakeExchanges(QString destSecretKey, QString Amount, QString SellCurrency, QString BuyCurrency)
{
	//Create Handles to destination and exchange accounts
	static SkySendAccount destAcc;
	static SkySendAccount exchangeAcc;

	destAcc.SetDBmanager(&mDB);
	destAcc.SetCurAccKeypair(destSecretKey);
	qDebug() << destAcc.GetCurAccKeypair()->getAccountId();

    exchangeAcc.SetDBmanager(&mDB);
	exchangeAcc.SetCurAccKeypair("SANRDX7FQAG3PTZT6EUJBEG2RCFNFR52LXTG2X7YIU7WY7LH2YTL6TUC");
	qDebug() << exchangeAcc.GetCurAccKeypair()->getAccountId();
	
	//The Algorithm is:
		//Sender makes offer to sell cur for lumens
		//Fullfill order - Use exchange account
		//Send Lumens to Destination
		//Dest makes offer to sell lumens for cur
		//Fullfill order
	
	//Due to the way signals work the connection to the function to be called have to be done before the 
	//functions that emit the signal are called so even though it seems that the algorithm is done in reverse 
	//it is done in the order above (and follows the numbering). 
	
	QObject::connect(&destAcc, &SkySendAccount::OfferSubmitted, [BuyCurrency, Amount]()
	{
		//(5)Fullfill order
		exchangeAcc.ManagerOffer(BuyCurrency, "Lumens", Amount, "1.00", false);
	});
	
	QObject::connect(&mUserAccount, &SkySendAccount::MoneySent, [this, BuyCurrency, Amount]()
	{
		//(4)Dest makes offer to sell lumens for cur
		destAcc.ManagerOffer("Lumens", BuyCurrency, Amount, "1.00");
	});

	QObject::connect(&exchangeAcc, &SkySendAccount::OfferSubmitted, [this, destSecretKey, Amount]()
	{
		//(3)Send Lumens to Destination
		mUserAccount.SendMoney(destSecretKey, Amount, "Lumens", "Lumens", false);
	});

	QObject::connect(&mUserAccount, &SkySendAccount::OfferSubmitted, [SellCurrency, Amount]() 
	{
		//(2)Fullfill order - Use exchange account
		exchangeAcc.ManagerOffer("Lumens", SellCurrency, Amount, "1.00"); 
	});
	
	//(1)Emits offer submited signal 
	mUserAccount.ManagerOffer(SellCurrency, "Lumens", Amount, "1.00");
}


void SkySend_DemoVS::CA_cancel_clicked()
{
	SetFrameVisible(ui.Init_Frame);
}

//Login Into Account events
void SkySend_DemoVS::LoginIntoAccount()
{
	Password = "";
	//Login Frame
	ui.Login_Frame->setGeometry(30, 110, 601, 481);
	SetFrameVisible(ui.Login_Frame);
}
void SkySend_DemoVS::Login_PasswordEncryption(QString inString)
{
	static bool passAlreadyCap = false;
	if (!passAlreadyCap && inString.length() >= 1)
	{
		Password += inString[inString.length() - 1];
		QString boxText = "";
		boxText.resize(inString.length(), '*');
		passAlreadyCap = true;
		ui.Login_passwIn->setText(boxText);
	}
	else
		passAlreadyCap = false;
}
void SkySend_DemoVS::Login_ok_clicked()
{
	QString email = ui.Login_emailIn->text();

	if (mDB.CheckInfo(email.toLower(), Password))
	{		
		InitLoginSetup(email);
	}
	else
	{
		Password = "";
		ui.Login_passwIn->setText("");
		QMessageBox Msgbox;
		Msgbox.setText("Login Info Not Found Or Incorrect");
		Msgbox.exec();
	}
}
void SkySend_DemoVS::InitLoginSetup(QString email)
{
	QObject::connect(&mUserAccount, SIGNAL(accountGood()), this, SLOT(Home_UpdateBalance()));
	QObject::connect(&mUserAccount, SIGNAL(accountUpdate()), this, SLOT(UpdateCurrencyAndBalance()));
	
	//Setup user info
	QString SecretKey = mDB.GetSecretKey(mDB.GetCustomerID(email));
	mUserAccount.SetCurAccKeypair(SecretKey);
	mUserAccount.InitBalances();
	
	curUserEmail = email;

	ui.AccountNameLabel->setText("Acc ID: " + mDB.GetName(email));
	ui.AccountNameLabel->setVisible(true);
	
	SetUpHomeScreen(curUserEmail);
	mAccountManager.PaymentWatcher(SecretKey);
}
void SkySend_DemoVS::Login_cancel_clicked()
{
	SetFrameVisible(ui.Init_Frame);
}

//Updates
void SkySend_DemoVS::UpdateCurrencyAndBalance()
{
	QVector<Balances> inBalance = mUserAccount.GetAllBalances();
	for (auto balance : inBalance)
	{
		if (balance.index == ui.Home_CurrencyChange->currentIndex())
		{
			ui.Home_Currency->setText(balance.AssetCode);
			QRegularExpression rx("([0-9])+.[0-9][0-9]");
			if (rx.match(balance.BalanceAmount).hasMatch())
			{
				ui.Home_Balance->setText(rx.match(balance.BalanceAmount).captured(0));
			}
			break;
		}
	}
}
void SkySend_DemoVS::SetUpHomeScreen(QString currUser)
{
	//Render Frame
	ui.Home_Frame->setGeometry(140, 80, 371, 351);
	SetFrameVisible(ui.Home_Frame);
}
void SkySend_DemoVS::Home_ChangeCurrency(int index)
{
	mUserAccount.GetBalance();
}
void SkySend_DemoVS::Home_SendMoney_clicked()
{
	SendMoney();
}
void SkySend_DemoVS::Home_UpdateBalance()
{
	QVector<Balances> inBalance = mUserAccount.GetAllBalances();
	UpdateCurrencyAndBalance();
	for (auto balance : inBalance)
	{
		ui.Home_CurrencyChange->addItem(balance.AssetCode);
		ui.MF_BuyAssetBox->addItem(balance.AssetCode);
		if (balance.BalanceAmount.toDouble() > 1)
		{
			ui.SM_CurrencySenders->addItem(balance.AssetCode);
			ui.MF_SellAssetBox->addItem(balance.AssetCode);
		}
	}
}
void SkySend_DemoVS::Home_MakeOffer_clicked()
{
	ui.MF_Frame->setGeometry(140, 120, 401, 351);
	SetFrameVisible(ui.MF_Frame);
}

void SkySend_DemoVS::SendMoney()
{
	ui.SM_emailIn->setText("");
	ui.SM_amountIn->setText("");

	//UI setup
	ui.SM_Frame->setGeometry(55, 100, 581, 421);
	SetFrameVisible(ui.SM_Frame);
	QObject::connect(&mUserAccount, &SkySendAccount::accountSuccess, this, [=]() {SetUpHomeScreen(curUserEmail); });
}
void SkySend_DemoVS::SM_ok_clicked()
{
	if (ValidateEmail(ui.SM_emailIn->text().toLower(), true))
	{
		QString DestSecretKey = mDB.GetSecretKey(mDB.GetCustomerID(ui.SM_emailIn->text().toLower()));
		QString AmountToSend = ui.SM_amountIn->text();
		QString SenderCurrency = ui.SM_CurrencySenders->currentText();
		QString ReceiptCurrency = ui.SM_CurrencyRecipient->currentText();

		if (!DestSecretKey.isEmpty())
		{
			if (AmountToSend.contains(QRegularExpression("^([0-9])+.[0-9][0-9]$")))
			{
				if(SenderCurrency!=ReceiptCurrency)
					MakeExchanges(DestSecretKey, AmountToSend, SenderCurrency, ReceiptCurrency);
				else
					mUserAccount.SendMoney(DestSecretKey, AmountToSend, SenderCurrency, ReceiptCurrency);
			}
			else
			{
				QMessageBox Msgbox;
				Msgbox.setText("Invalid Amount");
				Msgbox.exec();
			}
		}
		else
		{
			//TODO
			//CreatingAccount(ui.SM_emailIn->text().toLower(), ui.SM_emailIn->text().toLower());
		}
	}
	else
	{
		QMessageBox Msgbox;
		Msgbox.setText("Invalid Email");
		Msgbox.exec();
	}
}
void SkySend_DemoVS::SM_cancel_clicked()
{
	SetUpHomeScreen(curUserEmail);
}

void SkySend_DemoVS::MF_MakeOffer_clicked()
{
	QString SellAsset	= ui.MF_SellAssetBox->currentText();
	QString BuyAsset	= ui.MF_BuyAssetBox->currentText();
	QString Amount = ui.MF_AmountIn->text();
	QString Price = ui.MF_AmountIn_2->text();
	mUserAccount.ManagerOffer(SellAsset, BuyAsset, Amount, Price);
}
void SkySend_DemoVS::MF_Cancel_clicked()
{
	SetUpHomeScreen(curUserEmail);
}


//Validation
bool SkySend_DemoVS::ValidateName(QString inName)
{
	/*
	The Reg translates into:
	1.allow one ore more occurrences of any letter, digit or special symbols ._%+-
	Case insensitivity is guaranteed by QRegularExpression::CaseInsensitiveOption value of the second argument.
	*/
	QRegularExpression rx = QRegularExpression("^[a-zA-Z'\\s]+$", QRegularExpression::CaseInsensitiveOption);
	if (!rx.match(inName).hasMatch())
	{
		ui.CA_InvalidName->setText("<font color='red'>Invalid Name</font>");
		ui.CA_InvalidName->setVisible(true);
		return false;
	}
	return true;
}
bool SkySend_DemoVS::ValidateEmail(QString inEmail, bool EmailExists)
{
	/*
	The Reg translates into:
	1.allow one ore more occurrences of any letter, digit or special symbols ._%+-
	2.followed by @
	3.followed by one ore more occurrences of any letter, digit or special symbols ._
	4.followed by . (dot)
	5.followed by two, three or four letters
	Case insensitivity is guaranteed by QRegularExpression::CaseInsensitiveOption value of the second argument.
	*/
	QRegularExpression rx = QRegularExpression("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b", QRegularExpression::CaseInsensitiveOption);
	if (!rx.match(inEmail).hasMatch())
	{
		ui.CA_InvalidEmail->setText("<font color='red'>Invalid email</font>");
		ui.CA_InvalidEmail->setVisible(true);
		return false;
	}

	if (!EmailExists)
	{
		if (mDB.CheckUserExist(inEmail))
		{
			ui.CA_InvalidEmail->setText("<font color='red'>Email already exist</font>");
			ui.CA_InvalidEmail->setVisible(true);
			return false;
		}
	}
	else if(!mDB.CheckUserExist(inEmail))
	{
		return false;
	}
	return true;
}
bool SkySend_DemoVS::ValidatePassword(QString inPass)
{
	/*
	The Password Validatiom translates into:
	1.Check it has an upper case letter
	2.Check it as a lower case letter
	3.Check it has a number
	4.check it has one special char
	5.check is greater than 8 chars
	*/
	if ((!inPass.contains(QRegularExpression("[A-Z]")) ||
		!inPass.contains(QRegularExpression("[a-z]")) ||
		!inPass.contains(QRegularExpression("[0-9]")) ||
		!inPass.contains(QRegularExpression("\\W"))) &&
		(inPass.length() < 8))
	{
		ui.CA_InvalidPass->setText("<font color='red'>Invalid Password</font>");
		ui.CA_InvalidPass->setVisible(true);
		return false;
	}
	return true;
}