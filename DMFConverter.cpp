#include "dmf2esf.h"

using namespace std;

DMFConverter::DMFConverter(ESFOutput ** esfout) // ctor
{
    esf = *esfout;

    /* Initialize all the variables */
    SongType = 0;
    TickBase = 0;
    TickTime1 = 0;
    TickTime2 = 0;
    RegionType = 0;
    CurrPattern = 0;
    CurrRow = 0;
    SkipPattern = 0;
    NextPattern = 0;
    NextRow = 0;
    TotalRowsPerPattern = 0;
    TotalPatterns = 0;
    PatternList = 0;
    ArpTickSpeed = 0;

    ChannelCount = 0;
    RegionType = 0;

    //NoiseMode = PSG_WHITE_NOISE_HI;
    for(int i=0; i<10; i++)
    {
        Channels[i].Id = CHANNEL_FM1;
        Channels[i].Type = CHANNEL_TYPE_FM;
        Channels[i].ESFId = ESF_FM1;
        //Channels[i].DACEnabled = 0;
        Channels[i].EffectCount = 0;
        Channels[i].Note = 0;
        Channels[i].Octave = 0;
        Channels[i].ArpCounter = 0;
        Channels[i].ToneFreq = 0;
        Channels[i].NoteFreq = 0;
        Channels[i].LastFreq = 0;
        Channels[i].NewFreq = 0;
        Channels[i].Instrument = 0xff;
        Channels[i].NewInstrument = 0;
        Channels[i].Volume = 0xff;
        Channels[i].NewVolume = 0;
        Channels[i].SubtickFX = 0;
        Channels[i].Arp = EFFECT_OFF;
        Channels[i].Arp1 = 0;
        Channels[i].Arp2 = 0;
        Channels[i].Porta = EFFECT_OFF;
        Channels[i].PortaSpeed = 0;
        Channels[i].PortaNote = EFFECT_OFF;
        Channels[i].PortaNoteActive = 0;
        Channels[i].PortaNoteSpeed = 0;
        Channels[i].PortaNoteTarget = 0;
        Channels[i].Vibrato = EFFECT_OFF;
        Channels[i].VibratoActive = 0;
        Channels[i].VibratoFineDepth = 0;
        Channels[i].VibratoDepth = 0;
        Channels[i].VibratoSpeed = 0;
        Channels[i].VibratoOffset = 0;
        Channels[i].Tremolo = EFFECT_OFF;
        Channels[i].TremoloActive = 0;
        Channels[i].TremoloDepth = 0;
        Channels[i].TremoloSpeed = 0;
        Channels[i].TremoloOffset = 0;
        Channels[i].VolSlide = EFFECT_OFF;
        Channels[i].VolSlideValue = 0;
        Channels[i].Retrig = EFFECT_OFF;;
        Channels[i].RetrigSpeed = 0;
        Channels[i].NoteSlide = EFFECT_OFF;
        Channels[i].NoteSlideSpeed = 0;
        Channels[i].NoteSlideFinal = 0;
        Channels[i].NoteCut = EFFECT_OFF;
        Channels[i].NoteCutActive = 0;
        Channels[i].NoteCutOffset = 0;
        LoopState[i] = Channels[i];
    }

    LoopFound = 0;
    LoopPattern = 0;
    LoopRow = 0;
    LoopFlag = 0;

    DACEnabled = 0;
    PSGNoiseFreq = 0;

    return;
}

DMFConverter::~DMFConverter() // dtor
{
    /* Free any allocated memory */
    delete [] data;
    delete [] PatternList;
    return;
}

/** @brief Initializes module **/
bool DMFConverter::Initialize(const char* Filename)
{
    int i;
    uint32_t j;
    uint32_t k;

    /* Open file */
    streampos   file_size;
    ifstream file (Filename, ios::in|ios::binary|ios::ate);
    if (file.is_open())
    {
        fprintf(stderr, "Loading file: %s\n", Filename);
        file_size = file.tellg();
        comp_data = new char [file_size];
        file.seekg (0, ios::beg);
        file.read (comp_data, file_size);
        file.close();

        uLong comp_size = 16777216;
        data = new char [comp_size];

        /* Decompress the file with miniz */
        int res;
        #if DEBUG
            fprintf(stdout, "decompression buffer: %lu, original filesize: %d\n", comp_size, (int)file_size);
        #endif
        res = uncompress((Byte*) data, &comp_size, (Byte*) comp_data, file_size);
        if(res != Z_OK)
        {
            fprintf(stderr, "Failed to uncompress: ");
            if(res == Z_MEM_ERROR)
                fprintf(stderr, "Not enough memory.\n");
            else if(res == Z_BUF_ERROR)
                fprintf(stderr, "Not enough room in the output buffer.\n");
            else
                fprintf(stderr, "Invalid or corrupted module?\n");
            return 1;
        }
        else
        {
            delete [] comp_data;

            /* Decompression successful, now parse the DMF metadata */
            char DefleMagic[17];
            memcpy(DefleMagic, &data[0], 16);
            DefleMagic[16] = 0;
            res = strcmp(DefleMagic, ".DelekDefleMask.");
            if(res)
            {
                fprintf(stderr, "Not a valid DefleMask module.\n");
                return 1;
            }
            #if DEBUG
                fprintf(stdout, "Module version: %x\n", (int) data[16]);
                fprintf(stdout, "System: %x\n", (int) data[17]);
            #endif
            /* Get the DMF system. */
            System = (DMFSystem) data[17];
            if(System == DMF_SYSTEM_GENESIS)
            {
                /* This is a Genesis module */
                ChannelCount = 10;
                for(i=0;i<ChannelCount;i++)
                {
                    Channels[i].Id = MDChannels[i].aChannelId;
                    Channels[i].Type = MDChannels[i].aChannelType;
                    Channels[i].ESFId = MDChannels[i].aESFChannel;
                }
            }
            else if(System == DMF_SYSTEM_SMS)
            {
                fprintf(stderr, "Master System module support is untested.\n");
                /* This is a Master System module */
                ChannelCount = 4;
                for(i=0;i<ChannelCount;i++)
                {
                    Channels[i].Id = SMSChannels[i].aChannelId;
                    Channels[i].Type = SMSChannels[i].aChannelType;
                    Channels[i].ESFId = SMSChannels[i].aESFChannel;
                }
            }
            else
            {
                fprintf(stderr, "Only Sega Genesis modules are supported.\n"); // Bug: Should obviously be 'Mega Drive modules'
            }

            /* Skip some module metadata. */
            uint32_t ModPtr = 0x12;
            uint8_t NameChars;
            for(i=0;i<2;i++)
            {
                NameChars = data[ModPtr];
                ModPtr = ModPtr+NameChars+1;
            }
            ModPtr+=2; // skip highlights
            #if DEBUG
                fprintf(stdout, "Pointer position after highlights skip: %#x\n", ModPtr);
            #endif

            /* Extract useful module metadata */
            TickBase = data[ModPtr];
            TickTime1 = data[ModPtr+1];
            TickTime2 = data[ModPtr+2];
            RegionType = data[ModPtr+3];
            // custom HZ is ignored (4, 5, 6, 7)
            TotalRowsPerPattern = data[ModPtr+8];
            TotalPatterns = data[ModPtr+9];
            ArpTickSpeed = data[ModPtr+10];
            ModPtr+=11;

            /* Skip the pattern matrix */
            ModPtr+=(ChannelCount*TotalPatterns);

            /* Skip the instrument data */
            uint8_t TotalDmfInstruments = data[ModPtr];
            #if DEBUG
                fprintf(stdout, "Instrument data located at: %#x (total %d instruments)\n", ModPtr, (int) TotalDmfInstruments);
            #endif
            uint8_t DmfInstrumentMode;
            ModPtr++;
            for(i=0;i<TotalDmfInstruments;i++)
            {
                #if DEBUG
                    fprintf(stdout, " Instrument %d, position %#x, ", i, ModPtr);
                #endif
                NameChars = data[ModPtr];
                ModPtr = ModPtr+NameChars+1;
                DmfInstrumentMode = data[ModPtr];
                #if DEBUG
                    fprintf(stdout, "type %d\n", DmfInstrumentMode);
                #endif
                ModPtr++;
                if(DmfInstrumentMode==1)
                {
                    /* FM instrument */
                    if(OutputInstruments)
                    {
                        if(UseTables)
                            fprintf(stdout, "instr_%02x ; FM Instrument %d\n", InstrumentTable[i],i);
                        else
                            fprintf(stdout, "instr_%02x ; FM Instrument %d\n", i,i);
                        this->OutputFMInstrument(ModPtr);
                    }
                    ModPtr+=8+(19*4); // YM2612 has 4 operators
                }
                else if(DmfInstrumentMode==0)
                {
                    /* Standard instrument */
                    for(j=0;j<4;j++)
                    {
                        #if DEBUG
                            fprintf(stdout, "  macro %d, position %#x\n", j, ModPtr);
                        #endif
                        // Get size of envelope.
                        k=data[ModPtr];
                        // Undocumented! If the envelope size is 0, there's no data
                        // for the loop position.
                        if(k>0)
                            ModPtr+=(k*4)+2;
                        else
                            ModPtr++;
                        if(j==1)
                            ModPtr++; // Skip the arpeggio macro
                    }
                }
            }

            /* MD or SMS does not support wavetables */
            ModPtr++;

            /* Finally build the pattern offset table */
            PatternData = new uint32_t[ChannelCount];

            #if DEBUG
                fprintf(stdout, "Pattern data located at: %#x\n", ModPtr);
            #endif
            /* Get some pattern data */
            for(i=0;i<ChannelCount;i++)
            {
                Channels[i].EffectCount = data[ModPtr];
                #if DEBUG
                    fprintf(stdout, " Channel %d, effects: %d, position %#x\n",i,(int)Channels[i].EffectCount,ModPtr);
                #endif
                ModPtr++;
                PatternData[i] = ModPtr;
                ModPtr+=TotalPatterns*rowsize(i,TotalRowsPerPattern);

                /* Check for backwards jumps */
                for(CurrPattern=0;CurrPattern<TotalPatterns;CurrPattern++)
                {
                    for(CurrRow=0;CurrRow<TotalRowsPerPattern;CurrRow++)
                    {
                        uint32_t ChannelData = 0;
                        ChannelData = PatternData[i]+CurrPattern*rowsize(i,TotalRowsPerPattern);
                        ChannelData+= rowsize(i,CurrRow);
                        uint8_t EffectCounter;
                        for(EffectCounter=0;EffectCounter<Channels[i].EffectCount;EffectCounter++)
                        {
                            uint8_t EffectType=data[ChannelData+6+(EffectCounter*4)];
                            uint8_t EffectParam=data[ChannelData+8+(EffectCounter*4)];
                            if(EffectType == 0x0b) // jump
                            {
                                if(EffectParam < CurrPattern && LoopFound == false)
                                {
                                    LoopFound = true;
                                    LoopPattern = EffectParam;
                                    LoopRow = 0;
                                    fprintf(stdout, "backwards jump to %x detected in pattern %x\n",(int)LoopPattern,(int)CurrPattern);
                                }
                            }
                        }
                    }
                }
            }
            #if DEBUG
                fprintf(stdout, "End of pattern data at %#x\n",ModPtr);
            #endif
        }
    }
    else
    {
        fprintf(stderr, "File not found: %s\n", Filename);
        return 1;
    }
    return 0;
}

/** @brief Parses module and writes into ESF. **/
bool DMFConverter::Parse()
{
    uint8_t CurrChannel;
    uint32_t ChannelData;
    fprintf(stdout, "Now parsing pattern data...\n");
    NextRow = 0;
    for(CurrPattern=0;CurrPattern<TotalPatterns;CurrPattern++)
    {
        #if MODDATA
            fprintf(stdout, "Pattern %x\n",(int)CurrPattern);
        #endif
        CurrRow = NextRow;
        NextRow = 0;
        for(CurrRow=CurrRow;CurrRow<TotalRowsPerPattern;CurrRow++)
        {
            #if MODDATA
                fprintf(stdout, "Row %03d: ",(int)CurrRow);
            #endif
            esf->InsertPatRow(CurrPattern, CurrRow);

            /* Parse pattern data */
            for(CurrChannel=0;CurrChannel<ChannelCount;CurrChannel++)
            {
                //ChannelData = PatternList[chpat(CurrChannel,CurrPattern)]+rowsize(CurrChannel,CurrRow);
                ChannelData = PatternData[CurrChannel]+CurrPattern*rowsize(CurrChannel,TotalRowsPerPattern);
                ChannelData+= rowsize(CurrChannel,CurrRow);

                if(this->ParseChannelRow(CurrChannel,ChannelData))
                {
                    fprintf(stderr, "Could not parse module data.\n");
                    return 1;
                }
                //fprintf(stdout, "%#x, ", ChannelData);
            }
            #if MODDATA
                fprintf(stdout, "\n");
            #endif

            /* Set loop if we're at the loop start */
            if(LoopFound == true && LoopPattern == CurrPattern && LoopRow == CurrRow)
                esf->SetLoop();

            /* Calculate timing */
            uint8_t timing = TickTime1*(TickBase+1);
            if(CurrRow % 2 != 0)
                timing = TickTime2*(TickBase+1);

            /* Do Effects */
            for(int a=0;a<timing;a++)
            {

                esf->WaitCounter++;
            }

            /* Are we at the loop end? If so, start playing the loop row */
            if(LoopFlag == true)
            {
                #if MODDATA
                    fprintf(stdout, "Loop:    ");
                #endif
                for(CurrChannel=0;CurrChannel<ChannelCount;CurrChannel++)
                {
                    ChannelData = PatternData[CurrChannel]+LoopPattern*rowsize(CurrChannel,TotalRowsPerPattern);
                    ChannelData+= rowsize(CurrChannel,LoopRow);
                    if(this->ParseChannelRow(CurrChannel,ChannelData))
                        return 1;
                }
                #if MODDATA
                    fprintf(stdout, "\n");
                #endif
                esf->GotoLoop();
                break;
            }

            /* Do pattern jumps */
            if(SkipPattern)
            {
                SkipPattern = 0;
                CurrPattern = NextPattern-1;
                break;
            }
        }
        if(LoopFlag == true)
            break;
    }

    esf->StopPlayback();
    return 0;
}
/** @brief Parses pattern data for a single channel **/
bool DMFConverter::ParseChannelRow(uint8_t chan, uint32_t ptr)
{

    uint8_t EffectCounter;
    uint8_t EffectType;
    uint8_t EffectParam;

    /* Clean up effects */
    Channels[chan].NoteCut = EFFECT_OFF;
    Channels[chan].NoteDelay = EFFECT_OFF;

    /* Get row data */
    Channels[chan].Note = data[ptr];
    Channels[chan].Octave = data[ptr+2];
    Channels[chan].NewVolume = data[ptr+4];
    Channels[chan].NewInstrument = data[ptr+(Channels[chan].EffectCount*4)+6];

    /* Instrument updated? */
    if(Channels[chan].NewInstrument != Channels[chan].Instrument && Channels[chan].NewInstrument != 0xff)
    {
        Channels[chan].Instrument = Channels[chan].NewInstrument;
        if(UseTables)
            esf->SetInstrument(Channels[chan].ESFId,InstrumentTable[Channels[chan].Instrument]);
        else
            esf->SetInstrument(Channels[chan].ESFId,Channels[chan].Instrument);

        /* Echo resets the volume if the instrument is changed (FM only...) */
        if(Channels[chan].Type != CHANNEL_TYPE_PSG && Channels[chan].Type != CHANNEL_TYPE_PSG4)
            Channels[chan].Volume = 0x7f;

    }

    /* Volume updated? */
    if(Channels[chan].NewVolume != Channels[chan].Volume && Channels[chan].NewVolume != 0xff)
    {
        Channels[chan].Volume = Channels[chan].NewVolume;
        if(Channels[chan].Type == CHANNEL_TYPE_FM || CHANNEL_TYPE_FM6)
            esf->SetVolume(Channels[chan].ESFId,(Channels[chan].Volume));
        else if(Channels[chan].Type == CHANNEL_TYPE_PSG || CHANNEL_TYPE_PSG4)
            esf->SetVolume(Channels[chan].ESFId,(Channels[chan].Volume));
    }

    /* Parse some effects before any note ons */
    for(EffectCounter=0;EffectCounter<Channels[chan].EffectCount;EffectCounter++)
    {
        EffectType=data[ptr+6+(EffectCounter*4)];
        EffectParam=data[ptr+8+(EffectCounter*4)];
        if(EffectType == 0x17) // DAC enable
        {
            DACEnabled = 0;

            if(EffectParam > 0)
                DACEnabled = 1;

            //fprintf(stderr, "effect %02x%02x: DAC enable %d\n",(int)EffectType,(int)EffectParam,(int)DACEnabled);
        }
        else if(EffectType == 0x20) // Set noise mode
        {
            PSGNoiseFreq = 0;
            PSGPeriodicNoise = 0;

            if(EffectParam & 0xF0)
                PSGNoiseFreq = 1;
            if(EffectParam & 0x0F)
                PSGPeriodicNoise = 1;

            fprintf(stdout,"psg val = %d %d\n",(int) PSGNoiseFreq, (int) PSGPeriodicNoise);
            //fprintf(stderr, "effect %02x%02x: PSG noise mode %d %d\n",(int)EffectType,(int)EffectParam,(int)PSGNoiseFreq,(int)PSGPeriodicNoise);
        }
    }

    /* Is this a note off? */
    if(Channels[chan].Note == 100)
    {
        #if MODDATA
            fprintf(stdout, "OFF ");
        #endif
        esf->NoteOff(Channels[chan].ESFId);

        Channels[chan].ToneFreq = 0;
        Channels[chan].LastFreq = 0;
        Channels[chan].NewFreq = 0;
    }
    /* Note on? */
    else if(Channels[chan].Note != 0 && Channels[chan].Octave != 0)
    {
        /* Convert octave/note format to something that makes more sense */
        if(Channels[chan].Note == 12)
        {
            Channels[chan].Octave++;
            Channels[chan].Note = 0;
        }

        #if MODDATA
            fprintf(stdout, "%s%x ",NoteNames[Channels[chan].Note].c_str(),(int)Channels[chan].Octave);
        #endif

        /* Turn off the tone portamento */
        Channels[chan].PortaNote = EFFECT_OFF;

        /* Parse some effects that will affect the note on */
        for(EffectCounter=0;EffectCounter<Channels[chan].EffectCount;EffectCounter++)
        {
            EffectType=data[ptr+6+(EffectCounter*4)];
            EffectParam=data[ptr+8+(EffectCounter*4)];
            if(EffectType == 0x03) // Tone portamento.
                Channels[chan].PortaNote = EFFECT_SCHEDULE;
            else if(EffectType == 0xed) // Note delay.
            {
                Channels[chan].NoteDelay = EFFECT_SCHEDULE;
                Channels[chan].NoteDelayOffset = EffectParam;
            }
        }

        /* If note delay or tone portamento is off, send the note on command already! */
        if(Channels[chan].NoteDelay == EFFECT_OFF || Channels[chan].PortaNote == EFFECT_OFF)
            this->NoteOn(chan);

    }
    /* Note column is empty */
    else {
        #if MODDATA
            fprintf(stdout, "--- ");
        #endif
    }

    #if MODDATA
        fprintf(stdout, "%02x %02x, ",(int)Channels[chan].NewVolume,(int)Channels[chan].NewInstrument);
    #endif

    /* Parse effects */
    for(EffectCounter=0;EffectCounter<Channels[chan].EffectCount;EffectCounter++)
    {
        EffectType=data[ptr+6+(EffectCounter*4)];
        EffectParam=data[ptr+8+(EffectCounter*4)];
        //fprintf(stdout, "%02x %02x, ",(int)EffectType,(int)EffectParam);
        switch(EffectType)
        {
            /* The do-nothing effects */
        case 0xff: // No effect
        case 0x17: // DAC enable (already done)
        case 0x20: // PSG noise mode (already done)
            break;
            /* Normal effects */
        case 0x00: // Arpeggio
            if(EffectParam != 0)
            {
                Channels[chan].Arp = EFFECT_NORMAL;
                uint8_t ArpOct;
                uint8_t ArpNote;

                /* do first freq */
                ArpOct = Channels[chan].Octave;
                ArpNote = Channels[chan].Note + (EffectParam & 0xf0 >> 4);
                while(ArpNote > 12)
                {
                    ArpOct++;
                    ArpNote-=12;
                }

                /* PSG */
                if(Channels[chan].Type == CHANNEL_TYPE_PSG)
                    Channels[chan].Arp1 = PSGFreqs[ArpNote][ArpOct-2];
                else
                    Channels[chan].Arp1 = (Channels[chan].Octave<<11)|FMFreqs[Channels[chan].Note];

                Channels[chan].Arp2 = 0;

                /* do second freq */
                if((EffectParam & 0x0f) > 0)
                {
                    ArpOct = Channels[chan].Octave;
                    ArpNote = Channels[chan].Note + (EffectParam & 0xf0 >> 4);
                    while(ArpNote > 12)
                    {
                        ArpOct++;
                        ArpNote-=12;
                    }

                    /* PSG */
                    if(Channels[chan].Type == CHANNEL_TYPE_PSG)
                        Channels[chan].Arp2 = PSGFreqs[ArpNote][ArpOct-2];
                    else
                        Channels[chan].Arp2 = (Channels[chan].Octave<<11)|FMFreqs[Channels[chan].Note];
                }

            }
            else
            {
                Channels[chan].Arp = EFFECT_OFF;
            }
            break;
        case 0x01: // Portamento up
            break;
        case 0x02: // Portamento down
            break;
        case 0x03: // Tone portamento
            break;
        case 0x04: // Vibrato
            break;
        case 0x05: // Tone portamento + volume slide
            break;
        case 0x06: // Vibrato + volume slide
            break;
        case 0x07: // Tremolo
            break;
        case 0x08: // Set panning
            if(Channels[chan].Type == CHANNEL_TYPE_FM || Channels[chan].Type == CHANNEL_TYPE_FM6)
                esf->SetParams(Channels[chan].ESFId,(EffectParam & 0x10)<<3 | (EffectParam & 0x01)<<6);
            break;
        case 0x09: // Set speed 1
            TickTime1 = EffectParam;
            break;
        case 0x0f: // Set speed 2
            TickTime2 = EffectParam;
            break;
        case 0x0a: // Volume slide
            break;
        case 0x0c: // Note retrig
            break;
        case 0xe1: // Note slide up
            break;
        case 0xe2: // Note slide down
            break;
        case 0xe3: // Vibrato mode
            break;
        case 0xe4: // Fine vibrato depth
            break;
        case 0xec: // Note cut
            break;
            /* Global Effects */
        case 0x0b: // Position jump
            if(LoopFound == true && EffectParam < CurrPattern && LoopFlag == false)
                LoopFlag = true;
            break;
        case 0x0d: // Pattern break
            SkipPattern = true;
            if(CurrPattern != TotalPatterns)
                NextPattern = CurrPattern+1;
            else // should actually be the same as loop
                NextPattern = 0;
            NextRow = EffectParam;
            break;
            /* Unknown/unsupported effects */
        default:
            break;
        }
    }

    return 0;
}

void DMFConverter::NoteOn(uint8_t chan)
{
    /* Is this the PSG noise channel? */
    if (Channels[chan].Type == CHANNEL_TYPE_PSG4)
    {
        uint8_t NoiseMode = 0;

        /* Is periodic noise active? */
        if(PSGPeriodicNoise)
            NoiseMode = 4;

        /* Are we using the PSG3 frequency? */
        if(PSGNoiseFreq)
        {
            // if C-8 change it to B-7
            if(Channels[chan].Note == 0 && Channels[chan].Octave == 8)
            {
                Channels[chan].Octave--;
                Channels[chan].Note = 11;
            }
            Channels[chan].Octave--;

            /* Get the frequency value */
            Channels[chan-1].ToneFreq = PSGFreqs[Channels[chan].Note][Channels[chan].Octave];

            /* Only update frequency if it's not the same as the last */
            if(Channels[chan-1].LastFreq != Channels[chan-1].ToneFreq)
                esf->SetFrequency(Channels[chan-1].ESFId,Channels[chan-1].ToneFreq);

            Channels[chan-1].LastFreq = Channels[chan-1].ToneFreq;

            NoiseMode = NoiseMode + 3;

        }
        else {
            switch(Channels[chan].Note)
            {
            default:
            case 3: // D
                NoiseMode++;
            case 2: // C#
                NoiseMode++;
            case 1: // C
                break;
            }
            fprintf(stdout,"noisemode = %d\n",(int) NoiseMode);
        }

        esf->NoteOn(Channels[chan].ESFId,NoiseMode);
    }

    /* Skip if this is the PSG3 channel and its frequency value is already used for the noise channel */
    else if(!(Channels[chan].Id == CHANNEL_PSG3 && PSGNoiseFreq))
    {
        /* Calculate frequency */
        Channels[chan].ToneFreq = 0;    // Reset if this is for a channel where this doesn't make much sense.

        /* Is this an FM channel? */
        if(Channels[chan].Type == CHANNEL_TYPE_FM || (Channels[chan].Type == CHANNEL_TYPE_FM6 && DACEnabled == false))
        {
            Channels[chan].ToneFreq = (Channels[chan].Octave<<11)|FMFreqs[Channels[chan].Note];
        }
        /* PSG */
        else if(Channels[chan].Type == CHANNEL_TYPE_PSG)
        {
            Channels[chan].Octave--;
            Channels[chan].ToneFreq = PSGFreqs[Channels[chan].Note][Channels[chan].Octave];
        }

        /* Reset last tone / new tone freqs */
        Channels[chan].NoteFreq = Channels[chan].ToneFreq;
        Channels[chan].LastFreq = 0;
        Channels[chan].NewFreq = 0;

        if((Channels[chan].Type == CHANNEL_TYPE_FM6 && DACEnabled == true))
        {
            if(UseTables)
                esf->NoteOn(ESF_DAC,SampleTable[Channels[chan].Note]);
            else
                esf->NoteOn(ESF_DAC,Channels[chan].Note);
        }
        else
        {
            esf->NoteOn(Channels[chan].ESFId,Channels[chan].Note,Channels[chan].Octave);
        }
    }
    return;
}

/** Extracts FM instrument data and stores as params for the EIF macro */
void DMFConverter::OutputFMInstrument(uint32_t ptr)
{
    static int optable[] = {1,2,3,4};
    static uint8_t dttable[] = {7,6,5,0,1,2,3};

    int i, op;
    fprintf(stdout,"alg  = %d\n",(int) data[ptr]);
    fprintf(stdout,"fb   = %d\n",(int) data[ptr+2]);
    ptr+=9;

    for(i=0;i<4;i++)
    {
        op = optable[i];
        fprintf(stdout,"ar%d  = %d\n",op,(int) data[ptr+(19*i-1)+1]);  //AR
        fprintf(stdout,"dr%d  = %d\n",op,(int) data[ptr+(19*i-1)+3]);  //DR
        fprintf(stdout,"sr%d  = %d\n",op,(int) data[ptr+(19*i-1)+17]); //D2R
        fprintf(stdout,"rr%d  = %d\n",op,(int) data[ptr+(19*i-1)+9]);  //RR
        fprintf(stdout,"tl%d  = %d\n",op,(int) data[ptr+(19*i-1)+12]); //TL
        fprintf(stdout,"sl%d  = %d\n",op,(int) data[ptr+(19*i-1)+10]); //SL
        fprintf(stdout,"mul%d = %d\n",op,(int) data[ptr+(19*i-1)+8]); //MULT
        fprintf(stdout,"dt%d  = %d\n",op, dttable[(int)data[ptr+(19*i-1)+16]]); //DT
        fprintf(stdout,"rs%d  = %d\n",op,(int) data[ptr+(19*i-1)+15]); //RS
        fprintf(stdout,"ssg%d = $%02x\n",op,(int) data[ptr+(19*i-1)+18]);//SSG-EG
    }

    fprintf(stdout,"\teif\n; end of instrument\n");

    return;
}
