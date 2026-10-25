"""R26 read-only/stopped protocol and heartbeat/telemetry coexistence test.

Never sends ARM, SPEED, DRV or a nonzero target. DKEEP must reject while stopped.
"""
import argparse
import json
import statistics
import time
from pathlib import Path

import serial


def run(port_name, output):
    report={"port":port_name,"commands":[],"chunks":[],"lines":[],"phases":[],
            "safety":"STOP/DISARM and queries only; DKEEP must reject; no ARM/motion"}
    buffer=b""
    with serial.Serial(port_name,115200,timeout=.01,write_timeout=1) as port:
        def receive():
            nonlocal buffer
            data=port.read(max(1,min(port.in_waiting,4096)))
            if not data:return []
            now=time.monotonic()
            report["chunks"].append({"time":now,"hex":data.hex()})
            buffer+=data
            lines=[]
            while b"\n" in buffer:
                raw,buffer=buffer.split(b"\n",1)
                line=raw.decode("ascii","strict").strip()
                if not line:continue
                lines.append(line)
                report["lines"].append({"time":now,"line":line})
            return lines

        def command(text,prefix):
            started=time.monotonic()
            entry={"command":text,"start":started}
            report["commands"].append(entry)
            port.write(("\n"+text+"\n").encode("ascii"))
            entry["writeDone"]=time.monotonic()
            while time.monotonic()-started<3:
                for line in receive():
                    if line==prefix or line.startswith(prefix+","):
                        entry.update(reply=line,rttMs=1000*(time.monotonic()-started))
                        return line
                    if line.startswith("ERR,"):raise RuntimeError(line)
            raise TimeoutError(text)

        try:
            for text,prefix in [("STOP","ACK,STOP"),("DISARM","ACK,DISARM"),
                                ("TEL,0","ACK,TEL,0"),("TOF,0","ACK,TOF,0")]:
                command(text,prefix)
            assert command("VER","VER")=="VER,VGT6_BM50_ATTITUDE_R26"
            caps=command("CAPS","CAPS").split(",")
            assert "DKEEP" in caps and "LOG_DISABLED" in caps and "WLOG_US" not in caps
            for text,prefix in [("HLOG,STATUS","HLOG,STATUS"),("WLOG,STATUS","WLOG,STATUS")]:
                assert command(text,prefix).split(",")[2:]==["0"]*5
            for text in ["HLOG,START","WLOG,READ,0,8","WLOGINFO"]:
                assert command(text,"ERR,LOG,DISABLED")=="ERR,LOG,DISABLED"
            assert command("DKEEP","ERR,DKEEP")=="ERR,DKEEP,EXPIRED_OR_NOT_DIRECT"
            for text,prefix in [("UART","UART"),("LINK","LINK"),("STACK","STACK"),
                                ("PIDGET","PID"),("PS2STATUS","PS2"),("SPEEDSTATUS","TEL"),("ANGLESTATUS","ATEL")]:
                reply=command(text,prefix)
                if text=="SPEEDSTATUS":assert reply.split(",")[26]=="0"
                if text=="ANGLESTATUS":assert reply.split(",")[2]==reply.split(",")[12]=="0"
                if text=="PS2STATUS":assert reply.split(",")[2]=="0"
                if text=="STACK":assert reply.split(",")[4]=="1"
            for with_tof in [False,True]:
                command("TEL,1","ACK,TEL,1")
                command(f"TOF,{int(with_tof)}",f"ACK,TOF,{int(with_tof)}")
                start=time.monotonic();end=start+6;next_keep=start
                first=len(report["lines"])
                while time.monotonic()<end:
                    if time.monotonic()>=next_keep:
                        command("DKEEP","ERR,DKEEP,EXPIRED_OR_NOT_DIRECT")
                        next_keep+=.12
                    else:receive()
                samples=[x for x in report["lines"][first:] if x["line"].startswith("TEL,")]
                assert len(samples)>=8, "telemetry starved"
                assert all(x["line"].split(",")[26]=="0" for x in samples)
                gaps=[1000*(b["time"]-a["time"]) for a,b in zip(samples,samples[1:])]
                phase={"withTof":with_tof,"seconds":time.monotonic()-start,"telCount":len(samples),
                       "medianGapMs":statistics.median(gaps),"maxGapMs":max(gaps)}
                report["phases"].append(phase);print(json.dumps(phase),flush=True)
                assert max(gaps)<800,"telemetry gap exceeded stopped-test bound"
            report["passed"]=True
        except Exception as error:
            report["error"]=repr(error)
            raise
        finally:
            for text,prefix in [("STOP","ACK,STOP"),("DISARM","ACK,DISARM"),
                                ("TEL,0","ACK,TEL,0"),("TOF,0","ACK,TOF,0"),
                                ("SPEEDSTATUS","TEL"),("ANGLESTATUS","ATEL"),
                                ("UART","UART"),("LINK","LINK"),("STACK","STACK")]:
                try:command(text,prefix)
                except Exception as error:report.setdefault("cleanupErrors",[]).append(str(error))
            output.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")


if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--port",required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists():parser.error("output exists; use a fresh file")
    run(args.port,args.output)
