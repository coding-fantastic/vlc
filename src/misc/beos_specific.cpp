/*****************************************************************************
 * beos_init.cpp: Initialization for BeOS specific features 
 *****************************************************************************
 * Copyright (C) 1999-2001 VideoLAN
 * $Id: beos_specific.cpp,v 1.26 2002/09/30 18:30:28 titer Exp $
 *
 * Authors: Jean-Marc Dressler <polux@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/
#include <Application.h>
#include <Roster.h>
#include <Path.h>
#include <Alert.h>
#include <Message.h>
#include <Window.h>

#include <stdio.h>
#include <string.h> /* strdup() */
#include <malloc.h>   /* free() */

extern "C"
{
#include <vlc/vlc.h>
}

/*****************************************************************************
 * The VlcApplication class
 *****************************************************************************/
class VlcApplication : public BApplication
{
public:
    vlc_object_t *p_this;

    VlcApplication(char* );
    ~VlcApplication();

    virtual void ReadyToRun();
    virtual void AboutRequested();
    virtual void RefsReceived(BMessage* message);
    virtual void MessageReceived(BMessage* message);
    
private:
    BWindow*     fInterfaceWindow;
    BMessage*    fRefsMessage;
};

/*****************************************************************************
 * Static vars
 *****************************************************************************/
static char *         psz_program_path;

//const uint32 INTERFACE_CREATED = 'ifcr';  /* message sent from interface */
#include "../../modules/gui/beos/MsgVals.h"

extern "C"
{

/*****************************************************************************
 * Local prototypes.
 *****************************************************************************/
static void AppThread( vlc_object_t *p_appthread );

/*****************************************************************************
 * system_Init: create a BApplication object and fill in program path.
 *****************************************************************************/
void system_Init( vlc_t *p_this, int *pi_argc, char *ppsz_argv[] )
{
    p_this->p_vlc->p_appthread =
            (vlc_object_t *)vlc_object_create( p_this, sizeof(vlc_object_t) );

    /* Create the BApplication thread and wait for initialization */
    vlc_thread_create( p_this->p_vlc->p_appthread, "app thread", AppThread,
                       VLC_THREAD_PRIORITY_LOW, VLC_TRUE );
}

/*****************************************************************************
 * system_Configure: check for system specific configuration options.
 *****************************************************************************/
void system_Configure( vlc_t * )
{

}

/*****************************************************************************
 * system_End: destroy the BApplication object.
 *****************************************************************************/
void system_End( vlc_t *p_this )
{
    /* Tell the BApplication to die */
    be_app->PostMessage( B_QUIT_REQUESTED );

    vlc_thread_join( p_this->p_vlc->p_appthread );
    vlc_object_destroy( p_this->p_vlc->p_appthread );

    free( psz_program_path );
}

/*****************************************************************************
 * system_GetProgramPath: get the full path to the program.
 *****************************************************************************/
char * system_GetProgramPath( void )
{
    return( psz_program_path );
}

/* following functions are local */

/*****************************************************************************
 * AppThread: the BApplication thread.
 *****************************************************************************/
static void AppThread( vlc_object_t * p_this )
{
    VlcApplication *BeApp = new VlcApplication("application/x-vnd.Ink-vlc");
    vlc_object_attach( p_this, p_this->p_vlc );
    BeApp->p_this = p_this;
    BeApp->Run();
    vlc_object_detach( p_this );
    delete BeApp;
}

} /* extern "C" */

/*****************************************************************************
 * VlcApplication: application constructor
 *****************************************************************************/
VlcApplication::VlcApplication( char * psz_mimetype )
               :BApplication( psz_mimetype ),
                fInterfaceWindow( NULL ),
                fRefsMessage( NULL )
{
    /* Nothing to do, we use the default constructor */
}

/*****************************************************************************
 * ~VlcApplication: application destructor
 *****************************************************************************/
VlcApplication::~VlcApplication( )
{
    /* Nothing to do, we use the default destructor */
    delete fRefsMessage;
}

/*****************************************************************************
 * AboutRequested: called by the system on B_ABOUT_REQUESTED
 *****************************************************************************/
void VlcApplication::AboutRequested( )
{
    BAlert *alert;
    alert = new BAlert( VOUT_TITLE,
                        "BeOS " VOUT_TITLE "\n\n<www.videolan.org>",
                        "Ok" );
    alert->Go( NULL );
}

/*****************************************************************************
 * ReadyToRun: called when the BApplication is initialized
 *****************************************************************************/
void VlcApplication::ReadyToRun( )
{
    BPath path;
    app_info info; 

    /* Get the program path */
    be_app->GetAppInfo( &info ); 
    BEntry entry( &info.ref ); 
    entry.GetPath( &path ); 
    path.GetParent( &path );
    psz_program_path = strdup( path.Path() );

    /* Tell the main thread we are finished initializing the BApplication */
    vlc_thread_ready( p_this );
}

/*****************************************************************************
 * RefsReceived: called when files are sent to our application
 *               (for example when the user drops fils onto our icon)
 *****************************************************************************/
void VlcApplication::RefsReceived(BMessage* message)
{
	if (fInterfaceWindow)
		fInterfaceWindow->PostMessage(message);
	else {
		delete fRefsMessage;
		fRefsMessage = new BMessage(*message);
	}
}

/*****************************************************************************
 * MessageReceived: a BeOS applications main message loop
 *                  Since VlcApplication and interface are separated
 *                  in the vlc binary and the interface plugin,
 *                  we use this method to "stick" them together.
 *                  The interface will post a message to the global
 *                  "be_app" pointer when the interface is created
 *                  containing a pointer to the interface window.
 *                  In this way, we can keep a B_REFS_RECEIVED message
 *                  in store for the interface window to handle later.
 *****************************************************************************/
void VlcApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case INTERFACE_CREATED: {
			BWindow* interfaceWindow;
			if (message->FindPointer("window", (void**)&interfaceWindow) == B_OK) {
				fInterfaceWindow = interfaceWindow;
				if (fRefsMessage) {
					fInterfaceWindow->PostMessage(fRefsMessage);
					delete fRefsMessage;
					fRefsMessage = NULL;
				}
			}
			break;
		}
		default:
			BApplication::MessageReceived(message);
	}
}
