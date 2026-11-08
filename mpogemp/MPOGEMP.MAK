###############################################################################
##### This Makefile generates the "MAJORBBS.EXE" and all standard DLL's   #####
###############################################################################

.autodepend
.path.c=c:\wgserv\src
.path.h=c:\wgserv\src
.path.dll=c:\wgserv
.path.msg=c:\wgserv
.path.obj=c:\wgserv\phobj

###############################################################################
#####                           Build DLL's                               #####
###############################################################################
mpogemp.dll: \
     mpogemsg.h    mpogehlp.h    mpogeshp.h    mpogeplt.h                            \
     gemain.obj     gefuncs.obj   gecmds.obj    gecybs.obj   geplanet.obj \
     gelib.obj      gedroids.obj  gescan.obj\
     \wgserv\dlib\galimp.lib        \wgserv\dlib\gsblimp.lib                 \
     \run286\bc3\lib\phapi.lib     \wgserv\dlib\nodef.def
     ltdll mpogemp


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
gescan.obj: gescan.c
     ctdll gescan

###############################################################################
#####               Generate Headers for Compilation                      #####
###############################################################################

mpogemsg.h: mpogemsg.msg
     cd \wgserv
     wgsmsx mpogemsg -osrc
     cd \wgserv\src
mpogehlp.h: mpogehlp.msg
     cd \wgserv
     wgsmsx mpogehlp -osrc
     cd \wgserv\src
mpogeshp.h: mpogeshp.msg
     cd \wgserv
     wgsmsx mpogeshp -osrc
     cd \wgserv\src

mpogeplt.h: mpogeplt.msg
     cd \wgserv
     wgsmsx mpogeplt -osrc
     cd \wgserv\src
