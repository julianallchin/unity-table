
s.waitForBoot({

    // Define the paths to your sample files
    ~samplePaths = [
		"/Users/julianallchin/github/unity-table/samples/set4/Kick 1.wav",
        "/Users/julianallchin/github/unity-table/samples/set4/Drums Perc 1.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Drums Perc 2.wav",
        "/Users/julianallchin/github/unity-table/samples/set4/Drums Top 1.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Drums Top 2.wav",

		"/Users/julianallchin/github/unity-table/samples/set4/Basses 1.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Basses 2.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Basses 3.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Chords 1 (Long).wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Chords 2.wav",

		"/Users/julianallchin/github/unity-table/samples/set4/Piano 1.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Arp.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Atmos Pluck.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Atmos Pad 1.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Atmos Pad 2.wav",

		"/Users/julianallchin/github/unity-table/samples/set4/Lead 1 (Long).wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Lead 2 (Long).wav",
		"/Users/julianallchin/github/unity-table/samples/set4/Lead 3 (Long).wav",
		"/Users/julianallchin/github/unity-table/samples/set4/FX Sweeps 1.wav",
		"/Users/julianallchin/github/unity-table/samples/set4/FX Sweeps 2.wav",
    ];

    // Load the samples into buffers
    ~buffers = ~samplePaths.collect { |path| Buffer.read(s, path) };

    // Define control buses for volume
    ~volBuses = (1..20).collect { Bus.control(s, 1) };
    ~defaultVolume = 0.0; // Default volume level (1.0 is max, 0.0 is silence)

    // Set the initial volume for each sample
    ~volBuses.do { |bus| bus.set(~defaultVolume) };

    // Define a SynthDef to play samples in a loop with controllable volume
    SynthDef(\playSample, {
        arg out = 1, bufnum, volBus, loop = 1;
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
    ~synths = Array.newClear(8);

    // Function to create and play synths
    ~playSynths = {
        ~synths = ~buffers.collect { |buf, i|
            Synth(\playSample, [\bufnum, buf, \volBus, ~volBuses[i], \loop, 1])
        };
    };

    ~playSynths.value();

    // Define MIDI CC responders for each sample
    CCResponder({ |src, chan, num, val|
        if (chan == 0) {
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
