
#include <iostream>
#include <sodium.h>

using namespace std;

int main()
{
    if (sodium_init() < 0)
    {
        cout << "Failed to initialize libsodium." << endl;
        return 1;
    }

    const char* password = "TestPassword123";

    char passwordHash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            passwordHash,
            password,
            14,
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        cout << "Password hashing failed." << endl;
        return 1;
    }

    cout << "Password hashed successfully!" << endl;
    cout << "Hash: " << passwordHash << endl;

    if (crypto_pwhash_str_verify(
            passwordHash,
            password,
            14) == 0)
    {
        cout << "Password verification successful!" << endl;
    }
    else
    {
        cout << "Password verification failed." << endl;
        return 1;
    }

    return 0;
}
