NAME
====

**identity-confirm-private** -- generate validation token based on
application's private key.

SYNOPSIS
========

        virgil identity-confirm-private  [-o <file>] -d <arg> -t <arg> --app-key <file>
                                 [--app-key-password <arg>] [-V] [--]
                                 [--version] [-h]

DESCRIPTION
===========

Provides a helper methods to generate validation token based on
application's private key. It is required for the following operations:

1.  create a Private Virgil Card with a confirmed Identity;
2.  revoke a Private Virgil Card, a group of Cards;
3.  get a Private key from the Private Keys Service.

OPTIONS
=======

        -o <file>,  --validation-token <file>
         A Validation-token. If omitted, stdout is used.

        -d <arg>,  --identity <arg>
         (required)  Identity value

        -t <arg>,  --identity-type <arg>
         (required)  Identity type

        --app-key <file>
         (required)  Application Private key

        --app-key-password <arg>
         Password to be used for Application Private Key encryption.

        -V,  --VERBOSE
         Show detailed information

        --,  --ignore_rest
         Ignores the rest of the labeled arguments following this flag.

        --version
         Displays version information and exits.

        -h,  --help
         Displays usage information and exits.

RETURN VALUE
============

On success, *validated identity model*:

    {
        "type": "email",
        "value": "alice@gmail.com",
        "validation_token": *validation_token*
    }

is returned. On error, throw exeption.

EXAMPLES
========

1.  Generate validation-token:

        virgil identity-confirm-private -d alice@domain.com -t email -o validated-identity-private.txt --app-key application-private.key

SEE ALSO
========

virgil(1)  
card-create-private(1)  
card-revoke-private(1)  
private-key-get(1)