/*
evCmd - send a command to a running eveCSS engine (http://evecss.github.io)

Copyright (C) 2013  Jens Eden, PTB Germany

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtCore/QCoreApplication>
#include <Cmdclient.h>
#include "eveParameter.h"
#include "eveError.h"

#define EVECMD_VERSION "1.8"
#define DEFAULT_LOGLEVEL 3

int main(int argc, char *argv[])
{
    bool skipOne = false;
    int debuglevel=DEFAULT_LOGLEVEL;
    QString xmlFileName;
    QString logFileName;
    QString hostName = "localhost";
    int portNumber = 12345;

    if (argc <= 1){
        printf ("usage: eveCmd -f <xml-File> [-d debuglevel] [-h <hostname> (default localhost)] [-p <port> (default 12345)]\n");
        return (1);
    }
    // poor mans argument parsing
    for ( int i = 1; i < argc; i++ ) {
            if (skipOne){
                    skipOne = false;
                    continue;
            }
            bool ok;
            QString argument = QString(argv[i]);
            QString argumentNext;
            QString parameter;
            if (i+1 < argc) argumentNext = QString(argv[i+1]);

            if ( argument.startsWith("-v")){
                printf ("\neveCmd version %s\n\n", EVECMD_VERSION);
                exit(0);
            }
            if ( argument.startsWith("-d") ){
                    parameter = argument.remove(0,2).trimmed();
                    if (parameter.isEmpty()) {
                            parameter = argumentNext.trimmed();
                            skipOne = true;
                    }
                    debuglevel = parameter.toInt(&ok);
                    if (debuglevel < 0) debuglevel = 0 ;
                    if (ok) continue;
            }
            else if ( argument.startsWith("-p") ){
                    parameter = argument.remove(0,2).trimmed();
                    if (parameter.isEmpty()) {
                            parameter = argumentNext.trimmed();
                            skipOne = true;
                    }
                    portNumber = parameter.toInt(&ok);
                    if (ok) continue;
            }
            else if ( argument.startsWith("-f") ){
                    xmlFileName = argument.remove(0,2).trimmed();
                    if (xmlFileName.isEmpty()) {
                            xmlFileName = argumentNext.trimmed();
                            skipOne = true;
                    }
                    if (!xmlFileName.isEmpty()) continue;
            }
            else if ( argument.startsWith("-h") ){
                    hostName = argument.remove(0,2).trimmed();
                    if (hostName.isEmpty()) {
                            hostName = argumentNext.trimmed();
                            skipOne = true;
                    }
                    if (!hostName.isEmpty()) continue;
            }
            else if ( argument.startsWith("-l") ){
                    logFileName = argument.remove(0,2).trimmed();
                    if (logFileName.isEmpty()) {
                            logFileName = argumentNext.trimmed();
                            skipOne = true;
                    }
                    if (!logFileName.isEmpty()) continue;
            }
            printf ("\"%s\"?\n", argv[i]);
            printf ("usage: eveCmd -f <xml-File> [-d debuglevel] [-h <hostname> (default localhost)] [-p <port> (default 12345)]\n");
            return (1);
    }

    QCoreApplication a(argc, argv);

    eveError *error = new eveError( 0, debuglevel);

    eveParameter *paralist = new eveParameter();
    paralist->setParameter("port", QString().setNum(portNumber));
    paralist->setParameter("host", hostName);
    paralist->setParameter("version", EVECMD_VERSION);
    paralist->setParameter("filename", xmlFileName);
    Cmdclient myclient;

    return a.exec();
}
