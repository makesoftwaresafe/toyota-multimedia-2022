#define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/beeyond/PKCS12PBEKey.h"

using namespace beecrypt::beeyond;

PKCS12PBEKey::PKCS12PBEKey(const array<jchar>& password, const bytearray* salt, int iterationCount) : _pswd(password)
{
	if (salt)
		_salt = new bytearray(*salt);
	else
		_salt = 0;
	_iter = iterationCount;
	_enc = 0;
}

PKCS12PBEKey::~PKCS12PBEKey()
{
	delete _salt;
}

bool PKCS12PBEKey::operator==(const Key& compare) const throw ()
{
	const PBEKey* tmp = dynamic_cast<const PBEKey*>(&compare);
	if (tmp)
	{
		if (_pswd != tmp->getPassword())
			return false;

		if (_salt && tmp->getSalt())
		{
			if (*_salt != *tmp->getSalt())
				return false;
		}
		else if (_salt || tmp->getSalt())
			return false;
				
		if (_iter != tmp->getIterationCount())
			return false;

		return true;
	}
	return false;
}

PKCS12PBEKey* PKCS12PBEKey::clone() const
{
	return new PKCS12PBEKey(_pswd, _salt, _iter);
}

bytearray* PKCS12PBEKey::encode(const array<jchar>& password)
{
	int i;

	bytearray* result = new bytearray((password.size() + 1) * 2);

	for (i = 0; i < password.size(); i++)
	{
		(*result)[2*i  ] = (password[i] >> 8) & 0xff;
		(*result)[2*i+1] = (password[i]     ) & 0xff;
	}
	(*result)[2*i  ] = 0;
	(*result)[2*i+1] = 0;

	return result;
}

int PKCS12PBEKey::getIterationCount() const throw ()
{
	return _iter;
}

const array<jchar>& PKCS12PBEKey::getPassword() const throw ()
{
	return _pswd;
}

const bytearray* PKCS12PBEKey::getSalt() const throw ()
{
	return _salt;
}

const bytearray* PKCS12PBEKey::getEncoded() const throw ()
{
	if (!_enc)
		_enc = encode(_pswd);

	return _enc;
}

const String& PKCS12PBEKey::getAlgorithm() const throw ()
{
	static const String ALGORITHM("PKCS#12/PBE");
	return ALGORITHM;
}

const String* PKCS12PBEKey::getFormat() const throw ()
{
	static const String FORMAT("RAW");
	return &FORMAT;
}
