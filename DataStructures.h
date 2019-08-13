#pragma once
#include <QString>
struct Balances
{
	int index;
	QString BalanceAmount;
	QString AssetType;
	QString AssetCode;
	QString AssetIssuer;
};

struct StellarInfo
{
	QString AccID;
	QString PublicKey;
	QString SecretKey;
};