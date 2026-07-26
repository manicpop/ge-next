###############################################################################
##### This Makefile generates the "MAJORBBS.EXE" and all standard DLL's   #####
###############################################################################

.autodepend
.path.c=c:\wgserv\src
.path.h=c:\wgserv\src
.path.dll=c:\wgserv
.path.msg=c:\wgserv
.path.obj=c:\wgserv\gaobj

###############################################################################
#####                           Build DLL's                               #####
###############################################################################
mpoga.dll: \
     mpogamsg.h    mpogahlp.h    mpogashp.h                            \
     gemain.obj     gearena.obj   gefuncs.obj   gecmds.obj    gecybs.obj   geplanet.obj \
     gelib.obj      gescan.obj\
     \wgserv\dlib\galimp.lib        \wgserv\dlib\gsblimp.lib                 \
     \run286\bc3\lib\phapi.lib     \wgserv\dlib\nodef.def
     ltdll mpoga


###############################################################################
#####                 Build Objects For FASTPACE DLL                      #####
###############################################################################

gemain.obj: gemain.c
     ctdll -DGE_ARENA -n..\gaobj gemain
gearena.obj: gearena.c
     ctdll -DGE_ARENA -n..\gaobj gearena
gefuncs.obj: gefuncs.c
     ctdll -DGE_ARENA -n..\gaobj gefuncs
gecmds.obj: gecmds.c
     ctdll -DGE_ARENA -n..\gaobj gecmds
gecybs.obj: gecybs.c
     ctdll -DGE_ARENA -n..\gaobj gecybs
geplanet.obj: geplanet.c
     ctdll -DGE_ARENA -n..\gaobj geplanet
gelib.obj: gelib.c
     ctdll -DGE_ARENA -n..\gaobj gelib
gescan.obj: gescan.c
     ctdll -DGE_ARENA -n..\gaobj gescan

###############################################################################
#####               Generate Headers for Compilation                      #####
###############################################################################

mpogamsg.h: mpogamsg.msg
     cd \wgserv
     wgsmsx mpogamsg -osrc
     cd \wgserv\src
mpogahlp.h: mpogahlp.msg
     cd \wgserv
     wgsmsx mpogahlp -osrc
     cd \wgserv\src
mpogashp.h: mpogashp.msg
     cd \wgserv
     wgsmsx mpogashp -osrc
     cd \wgserv\src
