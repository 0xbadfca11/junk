CPPFLAGS = /nologo /GR- /MD /O2y- /Zi
LFLAGS = /nologo /DEBUG /INCREMENTAL:NO /MANIFEST:NO /PDBALTPATH:%_PDB% /RELEASE

run: exe.exe dll.dll
	exe

exe.exe: exe.obj
	link $(LFLAGS) $**
dll.dll: dll.obj dll.def
	$(CC) /LD $** /link $(LFLAGS)
clean:
	-del exe.exe dll.dll *.obj *.pdb *.lib *.exp