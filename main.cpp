#include "dmf2esf.h"

using namespace std;

bool OutputInstruments = false;
bool ASMOut = false;
bool ExCommands = false;

int main(int argc, char *argv[])
{
    int     InputId = 0;
    int     OutputId = 0;
    int     Opt = 0;

    OutputInstruments = false;
    ExCommands = false;

    DMFConverter * dmf;
    ESFOutput * esf;
    INIReader * ini;

    fprintf(stdout, "\nDMF2ESF ver %d.%d (built %s %s)\n", MAJORVER, MINORVER, __DATE__, __TIME__);
    fprintf(stdout, "Copyright 2013-2014 ctr.\n");
    fprintf(stdout, "Licensed under GPLv2, see LICENSE.txt.\n\n");
//    fprintf(stdout, "Not licensed under any license;\nyou may not do anything at all with this program.\n\n");

    if(argc > 1)
    {
        for(int i = 0; i < argc; ++i)
        {
            //printf( "argument %d = %s\n", i, argv[i] );
            /* Check for options */
            Opt = strcmp(argv[i], "-i");
            if(!Opt)
            {
                OutputInstruments = true;
                continue;
            }
            Opt = strcmp(argv[i], "-a");
            if(!Opt)
            {
                ASMOut = true;
                continue;
            }
            Opt = strcmp(argv[i], "-e");
            if(!Opt)
            {
                ExCommands = true;
                continue;
            }

            if(OutputId == 0)
            {
                if(InputId > 0)
                    OutputId = i;
                if(InputId == 0)
                    InputId = i;
            }
        }

    }

    if(InputId == 0) // no input file?
    {
        //print usage and exit
        fprintf(stderr, "Usage: dmf2esf <options> input <output>\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "\t-i : Output FM instrument data\n");
        fprintf(stderr, "\t-a : Output ESF as an assembly file\n");
        fprintf(stderr, "\t-e : Use EchoEx extended commands\n");
        fprintf(stderr, "Please read \"readme.md\" for further usage instructions.\n");

        for(int a=0;a<12*7;a++)
        {
            fprintf(stderr, "%d\n", fmfreq(a));
        }

    }
    else
    {
        const char *dot = strrchr(argv[InputId], '.');
        if(strcmp(dot,".ini") == 0)
        {
            fprintf(stdout, "Loading ini: %s\n",argv[InputId]);
            ini = new INIReader(argv[InputId]);

            if (ini->ParseError() < 0) {
                fprintf(stderr, "Failed to load ini: %s\n",argv[InputId]);
                return EXIT_FAILURE;
            }

            bool SearchFlag=true;
            int TrackIndex=0;
            char IniSection[64];
            string input;

            while(SearchFlag)
            {
                sprintf(IniSection,"%d",TrackIndex);
                fprintf(stdout, "======== Reading [%s] ========\n",IniSection);
                string input = ini->Get(IniSection,"input","");
                if(input.length() > 0)
                {
                    if(input.rfind(".") < 0)
                    {
                        fprintf(stderr, "Silly input\n");
                        return EXIT_FAILURE;
                    }

                    string output = ini->Get(IniSection,"output","");
                    bool TrackSFX = ini->GetBoolean(IniSection,"sfx",false);
                    if(output.length() == 0)
                        output = input.substr(0,input.rfind("."));

                    if(output.rfind(".") < 0)
                    {
                        fprintf(stderr, "Silly output\n");
                        return EXIT_FAILURE;
                    }

                    fprintf(stderr, "Converting %s to %s\n",input.c_str(), output.c_str());

                    esf = new ESFOutput(output);
                    dmf = new DMFConverter(&esf);

                    FindInstruments(IniSection,ini,dmf);

                    if(dmf->Initialize(input.c_str()))
                    {
                        fprintf(stderr, "Failed to initialize, aborting\n");
                        break;
                    }
                    if(dmf->Parse())
                    {
                        fprintf(stderr, "Conversion failed, aborting\n");
                        break;
                    }
                    fprintf(stdout, "Successfully converted, continuing.\n");
                    delete esf;
                    delete dmf;
                }
                else
                {
                    //fprintf(stderr, "invalid input '%s' (%d)\n",input.c_str(), input.length());
                    break;
                }
                TrackIndex++;
            }
            delete ini;
        }
        else
        {
            esf = new ESFOutput(string(argv[OutputId]));
            dmf = new DMFConverter(&esf);

            if(dmf->Initialize(argv[InputId]))
            {
                fprintf(stderr, "Aborting\n");
                return EXIT_FAILURE;
            }
            if(dmf->Parse())
            {
                fprintf(stderr, "Conversion aborted.\n");
                return EXIT_FAILURE;
            }

            delete dmf;
            delete esf;
        }
    }
    return EXIT_SUCCESS;
}
