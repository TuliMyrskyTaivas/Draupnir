////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	draupnir/CredentialsManager.cpp
//
// summary:	Implements the credentials manager class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CredentialsManager.h"
#include "Logger.h"

#include <botan/pkcs8.h>
#include <botan/data_src.h>

#include <algorithm>
#include <iterator>
#include <sstream>

// These are external references to the symbols created by OBJCOPY
extern uint8_t _binary_key_rsa_start[];
extern uint8_t _binary_key_rsa_end[];
extern uint8_t _binary_cert_rsa_start[];
extern uint8_t _binary_cert_rsa_end[];

namespace Draupnir
{
std::vector<uint8_t> certBuffer(_binary_cert_rsa_start, _binary_cert_rsa_end);
std::vector<uint8_t> keyBuffer(_binary_key_rsa_start, _binary_key_rsa_end);

////////////////////////////////////////////////////////////////////////////////////////////////////
CredentialsManager::CredentialsManager()
	: m_cert(certBuffer)
{
	Botan::DataSource_Memory keyDataSource(keyBuffer);
	m_key = Botan::PKCS8::load_key(keyDataSource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<Botan::Certificate_Store*> CredentialsManager::trusted_certificate_authorities(
	const std::string& type,
	const std::string& context)
{
	Logger::GetInstance().Debug() << "trusted certificate authorities are requested for "
		<< type << '/' << context;
	return { new Botan::Certificate_Store_In_Memory(m_cert) };
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<Botan::X509_Certificate> CredentialsManager::cert_chain(
	const std::vector<std::string>& certKeyTypes,
	const std::string& type,
	const std::string& context)
{
	std::vector<Botan::X509_Certificate> result;

	std::ostringstream buf;
	buf << "certificate chain is requested for " << type << '/' << context << ": ";
	std::copy(certKeyTypes.begin(),
		certKeyTypes.end(),
		std::ostream_iterator<std::string>(buf, ", "));
	Logger::GetInstance().Debug() << buf.str();

	if ("tls-server" == type)
		result.push_back(m_cert);

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Botan::Private_Key* CredentialsManager::private_key_for(
	const Botan::X509_Certificate& cert,
	const std::string& type,
	const std::string& context)
{
	Logger::GetInstance().Debug() << "private key is requested for " << type << '/'
		<< context << ": " << cert.fingerprint();

	if (m_cert == cert)
		return m_key.get();
	else
		return nullptr;
}
} // namespace Draupnir
