import http from 'node:http';
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.dirname(fileURLToPath(import.meta.url));
const port=Number(process.argv[2]||8870);
const types={'.html':'text/html; charset=utf-8','.mjs':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.svg':'image/svg+xml'};
http.createServer(async(req,res)=>{
  try{
    const url=new URL(req.url,'http://localhost');
    const relative=decodeURIComponent(url.pathname==='/'?'/index.html':url.pathname);
    const filename=path.resolve(root,'.'+relative);
    if(!filename.startsWith(root+path.sep)||!types[path.extname(filename)]){res.writeHead(404);res.end();return;}
    const data=await fs.readFile(filename);
    res.writeHead(200,{'Content-Type':types[path.extname(filename)],'Cache-Control':'no-store','X-Servo-Console':'1'});res.end(data);
  }catch{res.writeHead(404);res.end('Not found');}
}).listen(port,'127.0.0.1',()=>console.log(`Local: http://127.0.0.1:${port}/`));
