import express from "express";
import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const appRoot = path.resolve(__dirname, "..");

const app = express();
const port = Number(process.env.PORT || 8787);
const historyLimit = Number(process.env.HISTORY_LIMIT || 12);
const maxImageBytes = Number(process.env.MAX_IMAGE_BYTES || 12_000_000);
const dataFile = path.resolve(__dirname, process.env.DATA_FILE || "./data/history.json");
let writeQueue = Promise.resolve();

app.use(express.json({ limit: "16mb" }));
app.use((req, res, next) => {
    res.setHeader("Access-Control-Allow-Origin", "*");
    res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res.setHeader("Access-Control-Allow-Headers", "Content-Type");
    res.setHeader("Cache-Control", "no-store");
    if (req.method === "OPTIONS") {
        res.status(204).end();
        return;
    }
    next();
});

app.get("/", (req, res) => {
    res.sendFile(path.join(appRoot, "index.html"));
});

app.get("/app.js", (req, res) => {
    res.sendFile(path.join(appRoot, "app.js"));
});

app.get("/api/health", (req, res) => {
    res.json({ ok: true });
});

app.get("/api/history", async (req, res, next) => {
    try {
        const records = await readHistory();
        res.json({ items: recordsToItems(records) });
    } catch (error) {
        next(error);
    }
});

app.post("/api/upload", async (req, res, next) => {
    try {
        const image = req.body?.image;
        if (typeof image !== "string" || !image.startsWith("data:image/")) {
            res.status(400).json({ error: "image 必须是 data:image/... base64 字符串" });
            return;
        }
        if (Buffer.byteLength(image, "utf8") > maxImageBytes) {
            res.status(413).json({ error: "图片太大，请降低分辨率或压缩质量" });
            return;
        }

        const created = await appendImage(image);
        res.json({ ok: true, key: created.name });
    } catch (error) {
        next(error);
    }
});

app.use((error, req, res, next) => {
    console.error(error);
    res.status(502).json({ error: error.message || "中转服务失败" });
});

app.listen(port, "0.0.0.0", () => {
    console.log(`Transfer relay listening on http://0.0.0.0:${port}`);
    console.log(`History file: ${dataFile}`);
});

async function readHistory() {
    try {
        const text = await fs.readFile(dataFile, "utf8");
        const parsed = JSON.parse(text);
        return parsed && typeof parsed === "object" ? parsed : {};
    } catch (error) {
        if (error.code === "ENOENT") return {};
        throw error;
    }
}

function recordsToItems(records) {
    if (!records || typeof records !== "object") return [];
    return Object.keys(records)
        .sort()
        .map(key => ({ key, image: records[key] }));
}

async function appendImage(image) {
    return enqueueWrite(async () => {
        const records = await readHistory();
        const key = `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
        records[key] = image;
        trimRecords(records, historyLimit);
        await writeHistory(records);
        return { name: key };
    });
}

function trimRecords(records, limit) {
    const keys = Object.keys(records || {}).sort();
    const extra = Math.max(0, keys.length - limit);
    for (const key of keys.slice(0, extra)) {
        delete records[key];
    }
}

async function writeHistory(records) {
    await fs.mkdir(path.dirname(dataFile), { recursive: true });
    await fs.writeFile(dataFile, JSON.stringify(records), "utf8");
}

function enqueueWrite(task) {
    const next = writeQueue.then(task, task);
    writeQueue = next.catch(() => {});
    return next;
}
