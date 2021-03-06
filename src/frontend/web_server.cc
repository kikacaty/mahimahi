/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <iostream>
#include <unistd.h>

#include "web_server.hh"
#include "apache_configuration.hh"
#include "system_runner.hh"
#include "config.h"
#include "util.hh"
#include "exception.hh"

using namespace std;

WebServer::WebServer( const Address & addr, const string & working_directory, const string & record_path )
    : config_file_( "/tmp/replayshell_apache_config" ),
      moved_away_( false )
{
    cout << "Apache Config File: " << config_file_.name() << " listening on: " << addr.str() <<  endl;
    config_file_.write( apache_main_config );

    string path_prefix = PATH_PREFIX;
    config_file_.write("LoadModule unixd_module " + path_prefix + "/modules/mod_unixd.so\n");
    config_file_.write("LoadModule log_config_module " + path_prefix + "/modules/mod_log_config.so\n");
    config_file_.write("LoadModule mpm_prefork_module " + path_prefix + "/modules/mod_mpm_prefork.so\n");

    config_file_.write( "WorkingDir " + working_directory + "\n" );
    config_file_.write( "RecordingDir " + record_path + "\n" );

    /* if port 443, add ssl components */
    if ( addr.port() == 443 ) { /* ssl */
        config_file_.write( apache_ssl_config );
    }

    /* add pid file, log files, user/group name, and listen line to config file and run apache */
    config_file_.write( "PidFile /tmp/replayshell_apache_pid." + to_string( getpid() ) + "." + to_string( random() ) + "\n" );
    /* Apache will check if this file exists before clobbering it,
       so we think it's ok for Apache to write here as root */

    config_file_.write( "ServerName mahimahi.\n" );

    config_file_.write( "ErrorLog " + path_prefix + "/logs/apache_errors.log\n" );

    config_file_.write( "CustomLog " + path_prefix + "/logs/custom.log common\n" );

    config_file_.write( "User #" + to_string( getuid() ) + "\n" );

    config_file_.write( "Group #" + to_string( getgid() ) + "\n" );

    config_file_.write( "Listen " + addr.str() );

    run( { APACHE2, "-f", config_file_.name(), "-k", "start" } );
}

WebServer::WebServer( const Address & addr, const string & working_directory, const string & record_path, const string & ssl_key, const string & ssl_cert )
    : config_file_( "/tmp/replayshell_apache_config" ),
      moved_away_( false )
{
    cout << "Apache Config File: " << config_file_.name() << " listening on: " << addr.str() <<  endl;
    config_file_.write( apache_main_config );

    string path_prefix = PATH_PREFIX;
    config_file_.write("LoadModule unixd_module " + path_prefix + "/modules/mod_unixd.so\n");
    config_file_.write("LoadModule log_config_module " + path_prefix + "/modules/mod_log_config.so\n");
    config_file_.write("LoadModule mpm_prefork_module " + path_prefix + "/modules/mod_mpm_prefork.so\n");

    config_file_.write( "WorkingDir " + working_directory + "\n" );
    config_file_.write( "RecordingDir " + record_path + "\n" );

    /* if port 443, add ssl components */
    if ( addr.port() == 443 ) { /* ssl */
        string apache_ssl_custom_config = "LoadModule ssl_module " + std::string( MOD_SSL ) + "\nSSLEngine on\nSSLCertificateFile      " + ssl_cert + "\nSSLCertificateKeyFile " + ssl_key +"\n";
        config_file_.write( apache_ssl_custom_config );
    }

    /* add pid file, log files, user/group name, and listen line to config file and run apache */
    config_file_.write( "PidFile /tmp/replayshell_apache_pid." + to_string( getpid() ) + "." + to_string( random() ) + "\n" );
    /* Apache will check if this file exists before clobbering it,
       so we think it's ok for Apache to write here as root */

    config_file_.write( "ServerName mahimahi.\n" );

    config_file_.write( "ErrorLog /dev/null\n" );

    config_file_.write( "CustomLog /dev/null common\n" );

    config_file_.write( "User #" + to_string( getuid() ) + "\n" );

    config_file_.write( "Group #" + to_string( getgid() ) + "\n" );

    config_file_.write( "Listen " + addr.str() );

    run( { APACHE2, "-f", config_file_.name(), "-k", "start" } );
}

WebServer::~WebServer()
{
    if ( moved_away_ ) { return; }

    try {
        run( { APACHE2, "-f", config_file_.name(), "-k", "graceful-stop" } );
    } catch ( const exception & e ) { /* don't throw from destructor */
        print_exception( e );
    }
}

WebServer::WebServer( WebServer && other )
    : config_file_( move( other.config_file_ ) ),
      moved_away_( false )
{
    other.moved_away_ = true;
}
