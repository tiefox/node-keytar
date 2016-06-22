#include "keytar.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

namespace keytar {

bool AddPassword(const std::string& service,
                 const std::string& account,
                 const std::string& password) {
  OSStatus status = SecKeychainAddGenericPassword(NULL,
                                                  service.length(),
                                                  service.data(),
                                                  account.length(),
                                                  account.data(),
                                                  password.length(),
                                                  password.data(),
                                                  NULL);
  return status == errSecSuccess;
}

bool GetPassword(const std::string& service,
                 const std::string& account,
                 std::string* password) {
  void *data;
  UInt32 length;
  OSStatus status = SecKeychainFindGenericPassword(NULL,
                                                  service.length(),
                                                  service.data(),
                                                  account.length(),
                                                  account.data(),
                                                  &length,
                                                  &data,
                                                  NULL);
  if (status != errSecSuccess)
    return false;

  *password = std::string(reinterpret_cast<const char*>(data), length);
  SecKeychainItemFreeContent(NULL, data);
  return true;
}

bool DeletePassword(const std::string& service, const std::string& account) {
  SecKeychainItemRef item;
  OSStatus status = SecKeychainFindGenericPassword(NULL,
                                                   service.length(),
                                                   service.data(),
                                                   account.length(),
                                                   account.data(),
                                                   NULL,
                                                   NULL,
                                                   &item);
  if (status != errSecSuccess)
    return false;

  status = SecKeychainItemDelete(item);
  CFRelease(item);
  return status == errSecSuccess;
}

bool FindPassword(const std::string& service, std::string* password) {
  SecKeychainItemRef item;
  void *data;
  UInt32 length;

  OSStatus status = SecKeychainFindGenericPassword(NULL,
                                                   service.length(),
                                                   service.data(),
                                                   0,
                                                   NULL,
                                                   &length,
                                                   &data,
                                                   &item);
  if (status != errSecSuccess)
    return false;

  *password = std::string(reinterpret_cast<const char*>(data), length);
  SecKeychainItemFreeContent(NULL, data);
  CFRelease(item);
  return true;
}

bool FindIdentity(const std::string& commonName,
                  const std::string& passphrase,
                  std::string* pkcs12data) {
  CFMutableDictionaryRef query = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                             0,
                                                &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);


  CFDictionarySetValue(query, kSecReturnAttributes, kCFBooleanTrue);
  CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitAll);

  CFStringRef commonNameRef = CFStringCreateWithBytes(NULL,
                                          (const UInt8*)commonName.data(),
                                                        commonName.length(),
                                                        kCFStringEncodingUTF8,
                                                        false);
  CFDictionarySetValue(query, kSecClass, kSecClassIdentity);
  CFDictionarySetValue(query, kSecReturnRef, kCFBooleanTrue);
  CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);
  CFDictionarySetValue(query, kSecMatchSubjectContains, commonNameRef);

  CFTypeRef result = NULL;
  SecItemCopyMatching(query, &result);
  SecIdentityRef identity = (SecIdentityRef) result;

  SecItemImportExportKeyParameters keyParams;

  CFStringRef passphraseRef = CFStringCreateWithBytes(NULL,
                                          (const UInt8*)passphrase.data(),
                                                        passphrase.length(),
                                                        kCFStringEncodingUTF8,
                                                        false);
  keyParams.version = SEC_KEY_IMPORT_EXPORT_PARAMS_VERSION;
  keyParams.flags = 0;
  keyParams.passphrase = (CFTypeRef) passphraseRef;
  keyParams.alertTitle = NULL;
  keyParams.alertPrompt = NULL;
  keyParams.accessRef = NULL;
  // These two values are for import
  keyParams.keyUsage = NULL;
  keyParams.keyAttributes = NULL;
  CFDataRef exportedData = NULL;

  OSStatus oserr = SecItemExport(
                          identity,
                          kSecFormatPKCS12,
                          kSecItemPemArmour,
                          &keyParams,
                          &exportedData);
  if (oserr) {
     fprintf(stderr, "SecItemExport failed (oserr=%d)\n", oserr);
     return false;
  } else {
    CFStringRef exportedString =
    CFStringCreateFromExternalRepresentation(NULL,
                                     exportedData,
                            kCFStringEncodingUTF8);
    CFShow(exportedString);


    const CFIndex kCStringSize = 128;
    char temporaryCString[kCStringSize];
    bzero(temporaryCString, kCStringSize);
    CFStringGetCString(exportedString,
                     temporaryCString,
                         kCStringSize,
                kCFStringEncodingUTF8);
    pkcs12data = new std::string(temporaryCString);
    CFRelease(exportedData);
  }

  if (result != NULL) CFRelease(result);

  CFRelease(query);
  return true;
}

}  // namespace keytar
