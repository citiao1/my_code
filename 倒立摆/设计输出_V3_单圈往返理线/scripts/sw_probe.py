import sys,os
from pathlib import Path
r=Path(__file__).resolve().parents[2]/'.cad_runtime'
sys.path[:0]=[str(r),str(r/'win32'),str(r/'win32/lib'),str(r/'Pythonwin')]
os.add_dll_directory(str(r/'pywin32_system32'))
import pythoncom,win32com.client
pythoncom.CoInitialize()
tl=pythoncom.LoadTypeLib(r'D:\solidworks\SOLIDWORKS\sldworks.tlb')
print('tlb',tl.GetLibAttr(),flush=True)
mod=win32com.client.gencache.MakeModuleForTypelibInterface(tl)
print('module',mod,flush=True)
sw=mod.ISldWorks(win32com.client.Dispatch('SldWorks.Application')._oleobj_)
print('sw',sw,flush=True)
print('version',sw.RevisionNumber(),flush=True)
sw.Visible=True
