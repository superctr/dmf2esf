// Deflemask -> Echo Converter
// Started 2013-12-05, in Linköping, Sverige by Ian Karlsson.

/*
                        Useful Links:

Deflemask Manual (contains effect list)
    http://www.delek.com.ar/soft/deflemask/manual.html
Deflemask DMF format documentation (semi-accurate)
    http://www.delek.com.ar/soft/deflemask/DMF_SPECS.txt
Echo ESF documentation
    https://github.com/sikthehedgehog/Echo/blob/master/doc/esf.txt
*/

#ifndef DMF2ESF_H_INCLUDED
#define DMF2ESF_H_INCLUDED

#define MAJORVER 0
#define MINORVER 1

    #include <iostream>
    #include <iomanip>
    #include <fstream>
    #include <vector>
    #include <stdio.h>
    #include <math.h>
    #include <algorithm>
    #include <cstring>

    #define MINIZ_HEADER_FILE_ONLY
    #include "miniz.c"

    #include "ini.h"
    #include "inireader.h"

/* Main options */
extern bool OutputInstruments;
extern bool ASMOut;
extern bool ExCommands;

enum ChannelType
{
    CHANNEL_TYPE_INVALID,
    CHANNEL_TYPE_FM,    // Normal FM channel
    CHANNEL_TYPE_FM6,   // FM channel with DAC
    CHANNEL_TYPE_PSG,   // Normal PSG tone generator
    CHANNEL_TYPE_PSG4,
};

enum ChannelId
{
    CHANNEL_FM1,
    CHANNEL_FM2,
    CHANNEL_FM3,
    CHANNEL_FM4,
    CHANNEL_FM5,
    CHANNEL_FM6,
    CHANNEL_PSG1,
    CHANNEL_PSG2,
    CHANNEL_PSG3,
    CHANNEL_PSG4,
};

enum PSGNoiseMode
{
    PSG_WHITE_NOISE_HI = 0,
    PSG_WHITE_NOISE_MED = 1,
    PSG_WHITE_NOISE_LOW = 2,
    PSG_WHITE_NOISE_PSG3 = 3,
    PSG_PERIODIC_NOISE_HI = 4,
    PSG_PERIODIC_NOISE_MED = 5,
    PSG_PERIODIC_NOISE_LOW = 6,
    PSG_PERIODIC_NOISE_PSG3 = 7,
};

enum EffectMode
{
    EFFECT_OFF,     // all effects
    EFFECT_NORMAL,  // all effects except portamento, note slide
    EFFECT_UP,      // vibrato, portamento, note slide
    EFFECT_DOWN,    // vibrato, portamento, note slide
    EFFECT_SCHEDULE,// porta note
};

enum ESFChannel
{
    ESF_FM1 = 0x00,
    ESF_FM2 = 0x01,
    ESF_FM3 = 0x02,
    ESF_FM4 = 0x04,
    ESF_FM5 = 0x05,
    ESF_FM6 = 0x06,
    ESF_PSG1 =0x08,
    ESF_PSG2 =0x09,
    ESF_PSG3 =0x0a,
    ESF_PSG4 =0x0b,
    ESF_DAC = 0x0c,
};

static ChannelType ESFChannelTypes[] =
{
    CHANNEL_TYPE_FM,        //0
    CHANNEL_TYPE_FM,        //1
    CHANNEL_TYPE_FM,        //2
    CHANNEL_TYPE_INVALID,   //3
    CHANNEL_TYPE_FM,        //4
    CHANNEL_TYPE_FM,        //5
    CHANNEL_TYPE_FM,        //6
    CHANNEL_TYPE_INVALID,   //7
    CHANNEL_TYPE_PSG,       //8
    CHANNEL_TYPE_PSG,       //9
    CHANNEL_TYPE_PSG,       //a
    CHANNEL_TYPE_PSG4,      //b
    CHANNEL_TYPE_FM6,       //c
};

enum DMFSystem
{
    DMF_SYSTEM_GENESIS = 2,
    DMF_SYSTEM_SMS = 3,
};

struct ChannelArray
{
    ChannelId   aChannelId;
    ChannelType aChannelType;
    ESFChannel  aESFChannel;
};

static ChannelArray MDChannels[] =
{
    {CHANNEL_FM1, CHANNEL_TYPE_FM, ESF_FM1},
    {CHANNEL_FM2, CHANNEL_TYPE_FM, ESF_FM2},
    {CHANNEL_FM3, CHANNEL_TYPE_FM, ESF_FM3},
    {CHANNEL_FM4, CHANNEL_TYPE_FM, ESF_FM4},
    {CHANNEL_FM5, CHANNEL_TYPE_FM, ESF_FM5},
    {CHANNEL_FM6, CHANNEL_TYPE_FM6, ESF_FM6},
    {CHANNEL_PSG1, CHANNEL_TYPE_PSG, ESF_PSG1},
    {CHANNEL_PSG2, CHANNEL_TYPE_PSG, ESF_PSG2},
    {CHANNEL_PSG3, CHANNEL_TYPE_PSG, ESF_PSG3},
    {CHANNEL_PSG4, CHANNEL_TYPE_PSG4, ESF_PSG4},
};

static ChannelArray SMSChannels[] =
{
    {CHANNEL_PSG1, CHANNEL_TYPE_PSG, ESF_PSG1},
    {CHANNEL_PSG2, CHANNEL_TYPE_PSG, ESF_PSG2},
    {CHANNEL_PSG3, CHANNEL_TYPE_PSG, ESF_PSG3},
    {CHANNEL_PSG4, CHANNEL_TYPE_PSG4, ESF_PSG4},
};

static std::string NoteNames[] =
{
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-",
};
static std::string ESFChanNames[] =
{
    "FM 1", "FM 2", "FM 3", "", "FM 4", "FM 5", "FM 6", "", "PSG 1", "PSG 2", "PSG 3", "PSG 4", "PCM"
};

static uint16_t PSGFreqs[12][7] = // [semitone][octave]
{
    // 0   1   2   3   4   5,  6
    { 851,425,212,106, 53, 26, 13 }, // c
    { 803,401,200,100, 50, 25, 12 }, // c#
    { 758,379,189, 94, 47, 23, 11 }, // d
    { 715,357,178, 89, 44, 22, 10 }, // d#
    { 675,337,168, 84, 42, 21, 9 }, // e
    { 637,318,159, 79, 39, 19, 8 }, // f
    { 601,300,150, 75, 37, 18, 7 }, // f#
    { 568,284,142, 71, 31, 15, 6 }, // g
    { 536,268,134, 67, 33, 16, 5 }, // g#
    { 506,253,126, 63, 31, 15, 4 }, // a
    { 477,438,119, 59, 29, 14, 3 }, // a#
    { 450,225,112, 56, 28, 14, 2 }, // b
};
static uint16_t FMFreqs[12] =
{
    644,681,722,765,810,858,910,964,1021,1081,1146,1214
};

struct Channel
{

    ChannelId   Id;
    ChannelType Type;
    ESFChannel  ESFId;

    uint8_t     EffectCount;

    bool        NoteOn;

    uint8_t     Note;
    uint8_t     Octave;
    double      NoteFreq; // freq of last note on
    double      ToneFreq;
    uint8_t     NewNote;
    uint8_t     NewOctave;
    int16_t     LastFreq; // difference
    int16_t     NewFreq;  // difference
    uint8_t     Instrument;
    uint8_t     NewInstrument;
    uint8_t     Volume;
    uint8_t     NewVolume;
    uint8_t     SubtickFX;       // 0=none, >0=yes

    //0xx (arpeggio)
    EffectMode  Arp;
    uint16_t    Arp1;
    uint16_t    Arp2;
    uint8_t     ArpCounter;

    //1xx, 2xx (portamento)
    EffectMode  Porta;
    uint8_t     PortaSpeed;

    //3xx (porta to note), cancels out 1xx,2xx
    EffectMode  PortaNote;
    uint8_t     PortaNoteActive;
    uint8_t     PortaNoteSpeed;
    uint16_t    PortaNoteTarget;

    //4xx (vibrato)
    EffectMode  Vibrato;
    uint8_t     VibratoActive;
    uint8_t     VibratoFineDepth;
    uint8_t     VibratoDepth;
    uint8_t     VibratoSpeed;
    uint8_t     VibratoOffset;

    // 5xx, 6xx ignored
    // 7xx (tremolo)
    EffectMode  Tremolo;
    uint8_t     TremoloActive;
    uint8_t     TremoloDepth;
    uint8_t     TremoloSpeed;
    uint8_t     TremoloOffset;

    // 8xx (panning) doesn't need variables
    // Axx (volume slide)
    EffectMode  VolSlide;
    uint8_t     VolSlideValue;

    // Bxx (position jump, global effect)
    // Cxx (note retrig)
    EffectMode  Retrig;
    uint8_t     RetrigSpeed;

    // Dxx (pattern break, global effect)
    // E1xy, E2xy (note slide)
    EffectMode  NoteSlide;
    uint8_t     NoteSlideSpeed;
    uint8_t     NoteSlideFinal;

    // E3xx (set vibrato mode)
    // E4xx (set fine vibrato depth)
    // E5xx (fine tune, no support for now)
    // EBxx (set sample bank, no support for now)
    // ECxx (note cut)
    EffectMode  NoteCut;
    uint8_t     NoteCutActive;
    uint8_t     NoteCutOffset;

    // EDxx (note delay)
    EffectMode  NoteDelay;
    uint8_t     NoteDelayOffset;

    // EBxx (global fine tune, no support for now)
    // 17xx (DAC enable)

    // 10xy (set LFO, not supported by Echo)
    // 11xy, 12xx, 13xx ... (operator modifying effects not supported by Echo)
};

//=============================================================================

class ESFOutput
{
private:
    void    SetShortDelay(uint8_t length); // use WaitCounter
    void    SetDelay(uint8_t length);

public:
    std::ofstream ESFFile;

    uint32_t    WaitCounter; // just increase every time you want to wait...

    ESFOutput(std::string);             // ctor
    virtual ~ESFOutput();    // dtor

    void    Wait();
    void    NoteOn(ESFChannel chan,uint8_t note,uint8_t octave = 0);
    void    NoteOff(ESFChannel chan);
    void    SetVolume(ESFChannel chan,uint8_t volume);
    void    SetFrequency(ESFChannel chan,uint16_t freq);
    void    SetPSGNoise(uint8_t noise);
    void    SetInstrument(ESFChannel chan,uint8_t index);
    void    LockChannel(ESFChannel chan);
    void    SetParams(ESFChannel chan,uint8_t params);
    void    GotoLoop();
    void    SetLoop();
    void    StopPlayback();

    void    InsertPatRow(uint8_t pattern, uint8_t row);

    void    Close();
};

class DMFConverter
{
public:

    ESFOutput * esf;

    char*   comp_data;                  // compressed data
    char*   data;                       // uncompressed data

    uint32_t *   PatternData;           // pattern data table

    bool        UseTables;
    uint8_t     InstrumentTable[256];   // instrument conversion table
    uint8_t     SampleTable[12];

    DMFSystem   System;                 // current system
    uint8_t     ChannelCount;           // amount of channels
    uint8_t     RegionType;             // PAL or NTSC

    uint8_t     SongType;               // song type (BGM, SFX)

    uint8_t     TickBase;               // DMF timing
    uint8_t     TickTime1;
    uint8_t     TickTime2;

    uint8_t     CurrPattern;            // in pattern matrix.
    uint8_t     CurrRow;                // in pattern

    bool        SkipPattern;            // set if a command to skip the rest
                                        // of the pattern was found
    uint8_t     NextPattern;            // next pattern
    uint8_t     NextRow;                // row to start next pattern at

    uint8_t     TotalRowsPerPattern;
    uint8_t     TotalPatterns;
    uint32_t   * PatternList;
    uint8_t     ArpTickSpeed;           // arpeggio tick speed
//    PSGNoiseMode    NoiseMode;        // Set here because this option affects
                                        // multiple channels.
    bool        PSGNoiseFreq;           // 1: use PSG3 frequency
    bool        PSGPeriodicNoise;       // 1: use periodic noise

    bool        DACEnabled;             // Set here because the 17xx effect can be
                                        // done on any channel.

    Channel     Channels[10];
    Channel     LoopState[10];

    bool        LoopFound;
    uint8_t     LoopPattern;
    uint8_t     LoopRow;

    bool        LoopFlag;               // set to 1 when loop is found,
                                        // then 2 when we're ready to loop for real

    DMFConverter(ESFOutput ** esfout);             // ctor
    virtual     ~DMFConverter();    // dtor
    bool        Initialize(const char* Filename);     // load DMF
    bool        Parse();    // parse DMF
    bool        ParseChannelRow(uint8_t chan, uint32_t ptr); // parse channel
    void        ParseChannelEffects(uint8_t chan);
    void        NoteOn(uint8_t chan); // checks channel type and sends appropriate command to ESF
    void        OutputFMInstrument(uint32_t ptr); // outputs an FM instrument to stdout

    uint16_t    GetFreq(ChannelType chan);

};

/* Helper functions */
double divrest(double a, int b);
uint16_t fmfreq(double a);
uint16_t fmfreq2(long a);
void FindInstruments(char * inisection, INIReader *ini, DMFConverter *dmf);

//=============================================================================

// Some macros:
#define chpat(channel,pattern) (channel*TotalPatterns+pattern)
#define rowsize(channel,row) (row*(8+(4*Channels[channel].EffectCount)))
#define hexy(stream,var,pre) (stream<<pre<<setiosflags(ios::right)<<setbase(16)<<setfill('0')<<setw(2)<<(int)var<<setbase(0)<<resetiosflags(ios::right)<<setfill(' ')<<setw(0))

#endif // DMF2ESF_H_INCLUDED
