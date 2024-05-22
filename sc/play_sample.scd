s.waitForBoot({
    // Define the paths to your sample files
    ~samplePaths = [
        "/home/julianallchin/github/unity-table/samples/set2/Percs 1.wav",
		"/home/julianallchin/github/unity-table/samples/set2/Pad Chords.wav",
        "/home/julianallchin/github/unity-table/samples/set2/Synth Arp.wav",
        "/home/julianallchin/github/unity-table/samples/set2/Shaker.wav"
    ];

    // Load the samples into buffers
    ~buffers = ~samplePaths.collect { |path| Buffer.read(s, path) };

    // Define control buses for volume
    ~volBuses = (1..4).collect { Bus.control(s, 1) };
    ~defaultVolume = 1.0; // Default volume level (1.0 is max, 0.0 is silence)

    // Set the initial volume for each sample
    ~volBuses.do { |bus| bus.set(~defaultVolume) };

    // Define a SynthDef to play samples in a loop with controllable volume
    SynthDef(\playSample, {
        arg out = 0, bufnum, volBus, loop = 1;
        var sig, vol;

        // Get the volume from the control bus
        vol = In.kr(volBus);

        // Play the sample buffer with the volume control
        sig = PlayBuf.ar(2, bufnum, loop: loop) * vol;

        // Output the signal
        Out.ar(out, sig);
    }).add;

    // Initialize MIDI and set up the volume control
    MIDIClient.init;
	MIDIIn.connectAll;

    // Create an array to store the Synths
    ~synths = Array.newClear(4);

    // Function to create and play synths
    ~playSynths = {
        ~synths = ~buffers.collect { |buf, i|
            Synth(\playSample, [\bufnum, buf, \volBus, ~volBuses[i], \loop, 1])
        };
    };

    ~playSynths.value();

    // Define MIDI CC responders for each sample
    CCResponder({ |src, chan, num, val|
        if (chan == 0 and: (num >= 21 and: num <= 24)) {
            var index = num - 21;
            // Scale MIDI value (0-127) to volume range (0.0-1.0) and set the volume
            ~volBuses[index].set(val / 127.0);
        }
    });

    // Function to free all synths
    ~stopSynths = {
        ~synths.do { |synth| synth.free };
    };
});

// To stop the loops
// ~stopSynths.value();
