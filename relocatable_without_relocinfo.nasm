BITS  32

IMAGE_DOS_SIGNATURE                    EQU  "MZ"
IMAGE_NT_SIGNATURE                     EQU  "PE"
IMAGE_FILE_MACHINE_I386                EQU  0x014c
IMAGE_FILE_EXECUTABLE_IMAGE            EQU  0x0002
IMAGE_NT_OPTIONAL_HDR32_MAGIC          EQU  0x10b
IMAGE_SUBSYSTEM_WINDOWS_GUI            EQU  2
IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE  EQU  0x0040
IMAGE_NUMBEROF_DIRECTORY_ENTRIES       EQU  0x10
IMAGE_SCN_CNT_CODE                     EQU  0x00000020
IMAGE_SCN_MEM_EXECUTE                  EQU  0x20000000
IMAGE_SCN_MEM_READ                     EQU  0x40000000

IMAGE_DOS_HEADER:
DW	IMAGE_DOS_SIGNATURE
TIMES	58	DB	0
DD	IMAGE_NT_HEADERS

IMAGE_NT_HEADERS:
DD	IMAGE_NT_SIGNATURE

Machine               EQU  IMAGE_FILE_MACHINE_I386
NumberOfSections      EQU  2
TimeDateStamp         EQU  0
PointerToSymbolTable  EQU  0
NumberOfSymbols       EQU  0

IMAGE_FILE_HEADER:
DW	Machine
DW	NumberOfSections
DD	TimeDateStamp
DD	PointerToSymbolTable
DD	NumberOfSymbols
DW	IMAGE_SIZEOF_NT_OPTIONAL_HEADER
DW	IMAGE_FILE_EXECUTABLE_IMAGE

Magic                        EQU  IMAGE_NT_OPTIONAL_HDR32_MAGIC
MajorLinkerVersion           EQU  0
MinorLinkerVersion           EQU  0
SizeOfCode                   EQU  0
SizeOfInitializedData        EQU  0
SizeOfUninitializedData      EQU  0
AddressOfEntryPoint          EQU  text._start
BaseOfCode                   EQU  0
BaseOfData                   EQU  0
ImageBase                    EQU  0x00400000
SectionAlignment             EQU  0x1000
FileAlignment                EQU  SectionAlignment
MajorOperatingSystemVersion  EQU  0
MinorOperatingSystemVersion  EQU  0
MajorImageVersion            EQU  0
MinorImageVersion            EQU  0
MajorSubsystemVersion        EQU  4
MinorSubsystemVersion        EQU  0
Win32VersionValue            EQU  0
;SizeOfImage
;SizeOfHeaders
CheckSum                     EQU  0
Subsystem                    EQU  IMAGE_SUBSYSTEM_WINDOWS_GUI
DllCharacteristics           EQU  IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE
SizeOfStackReserve           EQU  0x100000
SizeOfStackCommit            EQU  0x10000
SizeOfHeapReserve            EQU  0x100000
SizeOfHeapCommit             EQU  0x10000
LoaderFlags                  EQU  0
NumberOfRvaAndSizes          EQU  IMAGE_NUMBEROF_DIRECTORY_ENTRIES

IMAGE_OPTIONAL_HEADER:
DW	Magic
DB	MajorLinkerVersion, MinorLinkerVersion
DD	SizeOfCode
DD	SizeOfInitializedData
DD	SizeOfUninitializedData
DD	AddressOfEntryPoint
DD	BaseOfCode
DD	BaseOfData
DD	ImageBase
DD	SectionAlignment
DD	FileAlignment
DW	MajorOperatingSystemVersion, MinorOperatingSystemVersion
DW	MajorImageVersion, MinorImageVersion
DW	MajorSubsystemVersion, MinorSubsystemVersion
DD	Win32VersionValue
DD	SizeOfImage
DD	SizeOfHeaders
DD	Win32VersionValue
DW	Subsystem
DW	DllCharacteristics
DD	SizeOfStackReserve
DD	SizeOfStackCommit
DD	SizeOfHeapReserve
DD	SizeOfHeapCommit
DD	LoaderFlags
DD	NumberOfRvaAndSizes

VirtualAddress  EQU  0
Size            EQU  0

IMAGE_DATA_DIRECTORY:
DD	VirtualAddress, Size
DD	rdata.IMAGE_IMPORT_DESCRIPTOR, rdata.IMAGE_SIZEOF_IMPORT_DESCRIPTOR
TIMES	(IMAGE_NUMBEROF_DIRECTORY_ENTRIES - 2)	DD	VirtualAddress, Size

IMAGE_SIZEOF_NT_OPTIONAL_HEADER  EQU  $ - IMAGE_OPTIONAL_HEADER

PointerToRelocations  EQU  0
PointerToLinenumbers  EQU  0
NumberOfRelocations   EQU  0
NumberOfLinenumbers   EQU  0

IMAGE_SECTION_HEADER:
DQ	".text"
DD	text.VirtualSize - text
DD	text
DD	text.VirtualSize - text
DD	text
DD	PointerToRelocations
DD	PointerToLinenumbers
DW	NumberOfRelocations
DW	NumberOfLinenumbers
DD	IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ

DQ	".rdata"
DD	rdata.VirtualSize - rdata
DD	rdata
DD	rdata.VirtualSize - rdata
DD	rdata
DD	PointerToRelocations
DD	PointerToLinenumbers
DW	NumberOfRelocations
DW	NumberOfLinenumbers
DD	IMAGE_SCN_MEM_READ

ALIGN	SectionAlignment
SizeOfHeaders:

text:
.GetEip:
	mov   ebp, [esp]
	ret
._start:
	mov   edi, esp
	sub   esp, 20
	xor   esi, esi
	call  .GetEip
.Eip:
	lea   ebx, [ebp + rdata.FORMAT_STRING - .Eip]
	push  esi
	call  [ebp + rdata._imp__GetModuleHandleW - .Eip]
	push  eax
	push  ebx
	push  edi
	call  [ebp + rdata._imp__wsprintfW - .Eip]
	push  esi
	push  esi
	push  edi
	push  esi
	call  [ebp + rdata._imp__MessageBoxW - .Eip]
	push  esi
	call  [ebp + rdata._imp__ExitProcess - .Eip]

.VirtualSize:
ALIGN	SectionAlignment

rdata:
.FORMAT_STRING:
DW	__UTF16__('%p'), 0

OriginalFirstThunk  EQU  0  ; "0 for terminating null import descriptor" is incorrect
;TimeDateStamp      EQU  0
ForwarderChain      EQU  0xffffffff  ; -1 if no forwarders
Name                EQU  0
FirstThunk          EQU  0

.IMAGE_IMPORT_DESCRIPTOR:
DD	OriginalFirstThunk
DD	TimeDateStamp
DD	ForwarderChain
DD	.KERNEL32
DD	.ImportAddressTableFromKernel32

DD	OriginalFirstThunk
DD	TimeDateStamp
DD	ForwarderChain
DD	.USER32
DD	.ImportAddressTableFromUser32

DD	OriginalFirstThunk
DD	TimeDateStamp
DD	ForwarderChain
DD	Name
DD	FirstThunk
.IMAGE_SIZEOF_IMPORT_DESCRIPTOR  EQU  $ - .IMAGE_IMPORT_DESCRIPTOR

.KERNEL32:
DB	"KERNEL32.DLL", 0

.USER32:
DB	"USER32.DLL", 0

.IMAGE_THUNK_DATA:
.ImportAddressTableFromKernel32:
._imp__GetModuleHandleW:
DD	.GetModuleHandleW
._imp__ExitProcess:
DD	.ExitProcess
DD	0

.ImportAddressTableFromUser32:
._imp__wsprintfW:
DD	.wsprintfW
._imp__MessageBoxW:
DD	.MessageBoxW
DD	0

Hint  EQU  0

.IMAGE_IMPORT_BY_NAME:

.GetModuleHandleW:
DW	Hint
DB	"GetModuleHandleW", 0

.ExitProcess:
DW	Hint
DB	"ExitProcess", 0

.wsprintfW:
DW	Hint
DB	"wsprintfW", 0

.MessageBoxW:
DW	Hint
DB	"MessageBoxW", 0

.VirtualSize:
ALIGN 	SectionAlignment

SizeOfImage: