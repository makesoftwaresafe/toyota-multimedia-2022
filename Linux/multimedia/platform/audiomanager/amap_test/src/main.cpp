/************************************************************************
 * @file: main.cpp
 *
 * @version: 1.1
 *
 * @description: This file is to start the media player process.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
 *
 * @copyright (c) 2010, 2011 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <assert.h>
#include <fstream>
#include <memory>
#include "CAmDltWrapper.h"
#include "CAmCommandLineSingleton.h"
#include "CAmSocketHandler.h"
#include "CAmInterface.h"
#include "CPlayer.h"
#include "CStdIn.h"


using namespace am;
using namespace std;

#define throw_assert(EXPRESSION, MESSAGE) \
        if (!(EXPRESSION)) throw std::runtime_error(MESSAGE)


//commandline options used by the application itself
TCLAP::ValueArg<string>        gApplication ("a","application","application name"             ,false,"player","string");
TCLAP::ValueArg<string>        gDbusNode    ("n","node"       ,"dbus node name (adapter)"     ,false,"application","string");
TCLAP::ValueArg<am_sourceID_t> gMainSourceID("q","playerID"   ,"main source id of player"     ,true, 1,"int");
TCLAP::ValueArg<string>        gDevice      ("d","device"     ,"ALSA playback device"         ,true,"entertainment_main","string");
TCLAP::ValueArg<am_sinkID_t>   gMainSinkID  ("s","amplifierID","main sink id e.g. amplifier"  ,true, 1,"int");
TCLAP::ValueArg<string>        gFile        ("f","file"       ,"single playback file"         ,false,"","string");
TCLAP::ValueArg<string>        gPlaylist    ("l","playlist"   ,"playlist in text format"      ,false,"","string");
TCLAP::SwitchArg               gStdOut      ("c","cli"        ,"console interaction");


bool readFile(CPlayer & player)
{
    bool continues = true;
    string line;
    ifstream list(gPlaylist.getValue());
    while (getline(list, line))
    {
        if (line.empty())
            continue;

        string::size_type seperator = line.find(';');
        if (seperator == string::npos) {
            player.add(line);
        }
        else {
            unsigned int timeout;
            istringstream(line.substr(seperator+1,line.length())) >> timeout;
            player.add(line.substr(0, seperator), timeout);
        }
    }
    return continues;
}

int main(int argc, char **argv)
{
    am_Error_e error = E_OK;

    //initialize the commandline parser, and add all neccessary commands
    try
    {
        TCLAP::CmdLine* cmd(CAmCommandLineSingleton::instanciateOnce("This is the Media Player for GENIVI AM"));
        cmd->add(gApplication);
        cmd->add(gDbusNode);
        cmd->add(gMainSourceID);
        cmd->add(gDevice);
        cmd->add(gMainSinkID);
        cmd->add(gFile);
        cmd->add(gPlaylist);
        cmd->add(gStdOut);
        cmd->parse(argc,argv);
    }
    catch (TCLAP::ArgException &e)  // catch any exceptions
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize the DLT logs with new process and register
    CAmDltWrapper::instanctiateOnce("CMP", gApplication.getValue().c_str());

    logInfo("GENIVI AM - Media Player");

    try
    {
        CAmSocketHandler *socketHandler = new CAmSocketHandler();
        CPlayer *player = new CPlayer(argc, argv, socketHandler);
        CAmInterface *interface = new CAmInterface(socketHandler, player, gDbusNode.getValue(),
                gApplication.getValue(), gMainSourceID.getValue(), gMainSinkID.getValue());
        CStdIn *input = NULL;

        if (gStdOut.getValue()) {
            input = new CStdIn(socketHandler, interface, player);
        }

        player->set(gDevice.getValue());
        player->setInterface(interface);

        if (gFile.isSet() && gPlaylist.isSet()) {
            string str("GENIVI AM Test - File and playlist specified!");
            cout << str << endl;
            logWarning(str);
        }

        vector<CPlayInfo> playlist;
        if (gFile.isSet())
            player->add(gFile.getValue());
        if (gPlaylist.isSet())
            readFile(*player);

        throw_assert(interface->connect(interface->getMainSrc(), interface->getMainSink()) == E_OK, "Connection not possible");
        socketHandler->start_listenting();
        delete interface;
        delete player;
        delete input;
        delete socketHandler;
    }
    catch (exception& exc)
    {
        stringstream ss;
        ss << "GENIVI AM Test - Exception thrown: " << exc.what();
        logError(ss.str());
        cout << ss.str() << endl;
        CAmDltWrapper::instance()->deinit();
        return EXIT_FAILURE;
    }

    logInfo("GENIVI AM Test - Ended Successfully");

    CAmDltWrapper::instance()->deinit();

    return error;
}



