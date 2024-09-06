###############################################################################
##### This Makefile generates the "MAJORBBS.EXE" and all standard DLL's   #####
###############################################################################

.autodepend
.path.c=c:\wgserv\src
.path.h=c:\wgserv\src
.path.dll=c:\wgserv
.path.msg=c:\wgserv\src
.path.obj=c:\wgserv\phobj

###############################################################################
#####                           Build DLL's                               #####
###############################################################################
mbmgemp.dll: \
     mbmgemsg.h    mbmgehlp.h                                         \
     gemain.obj     gefuncs.obj   gecmds.obj    gecybs.obj   geplanet.obj \
     gelib.obj      gedroids.obj \
     \wgserv\dlib\galimp.lib        \wgserv\dlib\gsblimp.lib                 \
     \run286\bc3\lib\phapi.lib     \wgserv\dlib\nodef.def
     ltdll mbmgemp


###############################################################################
#####                 Build Objects For FASTPACE DLL                      #####
###############################################################################

gemain.obj: gemain.c
     ctdll gemain
gefuncs.obj: gefuncs.c
     ctdll gefuncs
gecmds.obj: gecmds.c
     ctdll gecmds
gecybs.obj: gecybs.c
     ctdll gecybs
geplanet.obj: geplanet.c
     ctdll geplanet
gelib.obj: gelib.c
     ctdll gelib
gedroids.obj: gedroids.c
     ctdll gedroids

###############################################################################
#####               Generate Headers for Compilation                      #####
###############################################################################

mbmgemsg.h: mbmgemsg.msg
     ..\wgsmsx mbmgemsg
mbmgehlp.h: mbmgehlp.msg
     ..\wgsmsx mbmgehlp
mbmgeshp.h: mbmgeshp.msg
     ..\wgsmsx mbmgeshp
