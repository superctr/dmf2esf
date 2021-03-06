#include "dmf2esf.h"
using namespace std;

ESFOutput::ESFOutput(string Filename)
{
    WaitCounter = 0;

    /* Open file. ASM should be in text format, and binaries, well binary obviously */
    if(ASMOut)
        ESFFile.open(Filename.c_str(), ios_base::out | ios_base::trunc);
    else
        ESFFile.open(Filename.c_str(), ios_base::out | ios_base::trunc | ios_base::binary);

    if(!ESFFile.is_open())
    {
        fprintf(stderr, "Failed to open output. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    if(ASMOut)
    {
        //fprintf(ESFFile, "; Generated by DMF2ESF ver %d.%d (built %s %s)\n", MAJORVER, MINORVER, __DATE__, __TIME__);
        ESFFile << "; Generated by DMF2ESF ver "<<MAJORVER<<"."<<MINORVER<<" (built "<<__DATE__<<" "<<__TIME__")\n";
    }
    return;
}

ESFOutput::~ESFOutput() // dtor
{
    /* Cleanup */
    ESFFile.close();
}

void ESFOutput::Wait()
{
    while(WaitCounter > 255)
    {
        this->SetDelay(0);
        WaitCounter = WaitCounter - 256;
        //fprintf(stderr, "waitcounter a %d\n", WaitCounter);
    }

    if(WaitCounter == 0)
        return;

    if(WaitCounter <= 16 && ExCommands)
        this->SetShortDelay((uint8_t) WaitCounter);
    else
        this->SetDelay((uint8_t) WaitCounter);
    //fprintf(stderr, "waitcounter b %d\n", WaitCounter);
    WaitCounter = 0;
    return;
}

/** @brief Writes a Note On event to the ESF file */
void ESFOutput::NoteOn(ESFChannel chan, uint8_t note, uint8_t octave)
{
    this->Wait();

    uint8_t esfnote;
    uint8_t esfcmd = 0x00+(int)chan;

    switch(ESFChannelTypes[(int)chan])
    {
    case CHANNEL_TYPE_FM:
        esfnote = 32*octave+2*note+1;
        break;
    case CHANNEL_TYPE_PSG:
        esfnote = 24*octave+2*note;
        break;
    case CHANNEL_TYPE_PSG4:
    case CHANNEL_TYPE_FM6:
        esfnote = note;
        break;
    default:
        fprintf(stderr, "WARNING: Unknown channel type (wtf)\n");
        break;
    }
    if(ASMOut)
    {
        //fprintf(ESFFile, "\tdc.b $%02x, $%02x\t; Note on note %d, octave %d\n",(int)esfcmd,(int)esfnote,(int)note,(int)octave);
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        hexy(ESFFile,esfnote,", $");
        if(ESFChannelTypes[(int)chan] != CHANNEL_TYPE_FM6)
            ESFFile<<"\t; Note "<<NoteNames[note].c_str()<<(int)octave<<" on channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        else
            ESFFile<<"\t; Sample "<<(int)note<<" on channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        return;
    }
    // binary here
    ESFFile<<esfcmd<<esfnote;
    return;
}

void ESFOutput::NoteOff(ESFChannel chan)
{
    this->Wait();

    uint8_t esfcmd = 0x10+(int)chan;
    if(ASMOut)
    {
        //fprintf(ESFFile, "\tdc.b $%02x\t; Note off",(int)esfcmd);
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        ESFFile<<"\t\t; Note off channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        return;
    }
    ESFFile<<esfcmd;
    return;
}

void ESFOutput::SetVolume(ESFChannel chan, uint8_t volume)
{
    this->Wait();
    uint8_t esfcmd = 0x20+(int)chan;
    uint8_t esfvol;

    if(ESFChannelTypes[(int)chan] == CHANNEL_TYPE_PSG || ESFChannelTypes[(int)chan] == CHANNEL_TYPE_PSG4)
        esfvol = ~volume & 0x0f;
    else
        esfvol = ~volume & 0x7f;

    if(ASMOut)
    {
        //fprintf(ESFFile, "\tdc.b $%02x, $%02x\t; Volume",(int)esfcmd,(int)esfvol);
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        hexy(ESFFile,esfvol,", $");
        ESFFile<<"\t; Set volume for channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        return;
    }
    ESFFile<<esfcmd<<esfvol;
    return;
}
/*
ESF_SetFreq macro
    dc.b    $30+(\1)
    if      (\1)<ESF_PSG1
    dc.b    ((\2)<<3)|((\3)>>8)
    dc.b    (\3)&$FF
    elseif  (\1)<ESF_PSG4
    dc.b    (\2)&$0F
    dc.b    (\2)>>6
    else
    dc.b    (\2)
    endc
    endm

    byte = 0x30 + channel
    if channel < PSG1
        byte (octave<<3)|(frequency>>8)
        byte frequency&$ff
    elseif channel < PSG4
        byte frequency &$0f
        byte frequency >> 6
    else
        byte frequency
*/
void ESFOutput::SetFrequency(ESFChannel chan, uint16_t freq)
{
    this->Wait();
    uint8_t esfcmd = 0x30+(int)chan;
    uint8_t esffreq1 = 0;
    uint8_t esffreq2 = 0;
    bool    onebyte = false;

    switch(ESFChannelTypes[(int)chan])
    {
    case CHANNEL_TYPE_FM:
        esffreq1 = (uint8_t)(freq>>8);
        esffreq2 = (uint8_t)(freq & 0xFF);
        break;
    case CHANNEL_TYPE_PSG:
        esffreq1 = (uint8_t)(freq & 0x0F);
        esffreq2 = (uint8_t)(freq>>6);
        break;
    case CHANNEL_TYPE_PSG4:
        onebyte = true;
        esffreq1 = freq & 0x07;
        break;
    default:
        fprintf(stderr, "WARNING: Attempting to set frequency on a channel that doesn't support it\n");
        return;
        break;
    }

    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        hexy(ESFFile,esffreq1,", $");
        if(!onebyte)
            hexy(ESFFile,esffreq2,", $");
        ESFFile<<"\t; Set frequency '"<<std::dec<<(int)freq<<"' for channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        return;
    }
    ESFFile<<esfcmd<<esffreq1;
    if(!onebyte)
        ESFFile<<esffreq2;
    return;
}

void ESFOutput::SetInstrument(ESFChannel chan, uint8_t index)
{
    this->Wait();
    uint8_t esfcmd = 0x40+(int)chan;

    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        hexy(ESFFile,index,", $");
        ESFFile<<"\t; Set instrument for channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        return;
    }
    ESFFile<<esfcmd<<index;
    return;
}

void ESFOutput::LockChannel(ESFChannel chan)
{
    this->Wait();

    uint8_t esfcmd = 0xe0+(int)chan;
    if(ASMOut)
    {
        //fprintf(ESFFile, "\tdc.b $%02x\t; Note off",(int)esfcmd);
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        ESFFile<<"\t\t; Lock channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        return;
    }
    ESFFile<<esfcmd;
    return;
}

/** Warning: no error checking for non-FM channels */
void ESFOutput::SetParams(ESFChannel chan, uint8_t params)
{
    this->Wait();
    uint8_t esfcmd = 0xf0+(int)chan;
    uint8_t esfparams = params & 0xC0;

    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        hexy(ESFFile,esfparams,", $");
        ESFFile<<"\t; Set params for channel "<<ESFChanNames[(int)chan].c_str()<<"\n";
        return;
    }
    ESFFile<<esfcmd<<esfparams;
    return;
}

void ESFOutput::GotoLoop()
{
    this->Wait();
    uint8_t esfcmd = 0xfc;

    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        ESFFile<<"\t; Goto loop\n";
        return;
    }
    ESFFile<<esfcmd;
    return;
}

void ESFOutput::SetLoop()
{
    this->Wait();
    uint8_t esfcmd = 0xfd;

    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        ESFFile<<"\t; Set loop\n";
        return;
    }
    ESFFile<<esfcmd;
    return;
}

void ESFOutput::StopPlayback()
{
    this->Wait();
    uint8_t esfcmd = 0xff;

    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        ESFFile<<"\t; The End\n";
        return;
    }
    ESFFile<<esfcmd;
    return;
}

/** @brief Set delay  */
void ESFOutput::SetDelay(uint8_t length)
{
    uint8_t esfcmd = 0xfe;
    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        hexy(ESFFile,length,", $");
        ESFFile<<"\t; Delay\n";
        //fprintf(stderr, "DELAY %d\n", (int)length);
        return;
    }
    ESFFile<<esfcmd<<length;
    return;
}

/** @brief Set short delay (EXTENDED COMMAND) */
void ESFOutput::SetShortDelay(uint8_t length)
{
    uint8_t esfcmd = 0xd0 | ((length-1)& 0x0f);
    if(ASMOut)
    {
        ESFFile<<"\tdc.b ";
        hexy(ESFFile,esfcmd,"$");
        hexy(ESFFile,length,", $");
        ESFFile<<"\t; Short delay\n";
        //fprintf(stderr, "DELAY %d\n", (int)length);
        return;
    }
    ESFFile<<esfcmd;
    return;
}

/** @brief Inserts a comment containing pattern and row number to the ASM output */
void ESFOutput::InsertPatRow(uint8_t pattern, uint8_t row)
{
    if(ASMOut)
    {
        ESFFile<<"; ";
        hexy(ESFFile,pattern,"Pattern $");
        ESFFile<<", Row "<<(int)row<<"; ";
        ESFFile<<"\n";
        //fprintf(stderr, "DELAY %d\n", (int)length);
        return;
    }
    /* Do nothing if writing a binary */
    return;
}
