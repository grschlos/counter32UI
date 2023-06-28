# counter32UI
32-channel counter user interface


Usage:


    $ python3 client.py --mode [text/bin]


Text mode is used by default.


In order to use the script in the text mode please build and start the listener first:


    $ make
    $ ./listen&


Counter command examples:


Initialize the counter:

    hi&

Reset the counter:

    reset&

Stop the counter:

    interrupt&

Set the threshold of 2000 DAC units and count number of pulses at channel 1 within 1 second:

    dac=2000&chs=80000000&time=1&step=1&nSteps=1&

For binary commands format please refer to https://grschlos.github.io/counter/
