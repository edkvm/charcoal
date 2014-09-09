# charcoal

A basic remote shell


## Build

You can build charcoal using gcc:
    
    $ gcc src/*.c -o bin/charcoal

## Using
    
Run the server:
    
    $ ./charcoal server <port>
    
Connect with a client:
    
    $ ./charcoal client <hostname> <port>