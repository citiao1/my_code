(function () {
    const API_STORAGE_KEY = "transferRelayApiBase";
    const HISTORY_POLL_MS = 1500;
    const MAX_WIDTH = 1920;

    let currentRotation = 0;
    let historyData = {};
    let historyPollTimer = null;
    let currentHistorySignature = "";
    let scale = 1;
    let isDragging = false;
    let startX;
    let startY;
    let translateX = 0;
    let translateY = 0;

    function normalizeApiBase(value) {
        return (value || "").trim().replace(/\/+$/, "");
    }

    function getApiBase() {
        const sameOriginRelay =
            /^https?:$/.test(window.location.protocol) &&
            window.location.hostname !== "appassets.androidplatform.net"
                ? window.location.origin
                : "";
        return normalizeApiBase(
            window.TRANSFER_API_BASE ||
            localStorage.getItem(API_STORAGE_KEY) ||
            new URLSearchParams(window.location.search).get("api") ||
            sameOriginRelay
        );
    }

    function setStatus(id, text, color) {
        const el = document.getElementById(id);
        if (!el) return;
        el.innerText = text;
        if (color) el.style.color = color;
    }

    function setApiBase(value) {
        const next = normalizeApiBase(value);
        if (!next) return false;
        localStorage.setItem(API_STORAGE_KEY, next);
        return true;
    }

    function ensureApiBase() {
        const apiBase = getApiBase();
        if (apiBase) return apiBase;

        const value = prompt("请输入中转接口地址，例如：https://your-domain.com");
        if (!setApiBase(value)) {
            setStatus("upload-status", "请先设置中转接口地址", "#ffcc00");
            setStatus("status-text", "请先设置中转接口地址", "#ffcc00");
            return "";
        }
        return getApiBase();
    }

    async function apiFetch(path, options) {
        const apiBase = ensureApiBase();
        if (!apiBase) throw new Error("未设置中转接口地址");

        const response = await fetch(apiBase + path, {
            ...options,
            headers: {
                "Content-Type": "application/json",
                ...(options && options.headers ? options.headers : {})
            }
        });

        const body = await response.json().catch(() => ({}));
        if (!response.ok) {
            throw new Error(body.error || `接口请求失败：${response.status}`);
        }
        return body;
    }

    function addRelaySettingsButton() {
        const button = document.createElement("button");
        button.type = "button";
        button.innerText = "接口";
        button.style.cssText = [
            "position:fixed",
            "right:12px",
            "top:12px",
            "z-index:2000",
            "border:1px solid rgba(255,255,255,.25)",
            "background:rgba(0,0,0,.55)",
            "color:#fff",
            "border-radius:10px",
            "padding:8px 10px",
            "font-size:13px"
        ].join(";");
        button.onclick = function () {
            const current = getApiBase();
            const value = prompt("中转接口地址", current || "https://your-domain.com");
            if (setApiBase(value)) {
                location.reload();
            }
        };
        document.body.appendChild(button);
    }

    function init() {
        addRelaySettingsButton();
        if (window.innerWidth > 800) {
            initPC();
        } else {
            initPhone();
        }
    }

    function initPC() {
        document.getElementById("pc-area").style.display = "flex";
        loadHistoryLoop();
        historyPollTimer = setInterval(loadHistoryLoop, HISTORY_POLL_MS);
        window.addEventListener("beforeunload", function () {
            if (historyPollTimer) clearInterval(historyPollTimer);
        });
    }

    async function loadHistoryLoop() {
        try {
            const result = await apiFetch("/api/history");
            const items = Array.isArray(result.items) ? result.items : [];
            const signature = items.map(item => item.key).join("|");
            if (signature === currentHistorySignature) return;

            currentHistorySignature = signature;
            const data = {};
            items.forEach(item => {
                data[item.key] = item.image;
            });

            historyData = data;
            updateSidebar(data);

            const keys = Object.keys(data);
            if (keys.length) {
                const latestKey = keys[keys.length - 1];
                showImage(data[latestKey], latestKey);
                setStatus("status-text", "已连接中转接口", "#28a745");
            } else {
                setStatus("status-text", "等待手机发送图片", "#666");
            }
        } catch (error) {
            console.error(error);
            setStatus("status-text", "中转接口连接失败：" + error.message, "#d93025");
        }
    }

    function updateSidebar(data) {
        const listEl = document.getElementById("history-list");
        listEl.innerHTML = '<div class="history-title">历史记录（点选切换）</div>';

        const keys = Object.keys(data).reverse();
        keys.forEach(key => {
            const imgData = data[key];
            const div = document.createElement("div");
            div.className = "history-item";
            div.id = "thumb-" + key;
            div.innerHTML = `<img src="${imgData}" loading="lazy">`;
            div.onclick = () => showImage(imgData, key);
            listEl.appendChild(div);
        });
    }

    function showImage(base64Data, key) {
        const mainImg = document.getElementById("received-img");
        if (mainImg.getAttribute("data-key") !== key) {
            currentRotation = 0;
            mainImg.style.transform = "rotate(0deg)";
            mainImg.setAttribute("data-key", key);
        }

        mainImg.src = base64Data;
        document.getElementById("download-link").href = base64Data;

        document.querySelectorAll(".history-item").forEach(el => el.classList.remove("active"));
        const activeThumb = document.getElementById("thumb-" + key);
        if (activeThumb) activeThumb.classList.add("active");
    }

    window.rotateImage = function (deg) {
        const img = document.getElementById("received-img");
        if (!img.src) return;
        currentRotation += deg;
        img.style.transform = `rotate(${currentRotation}deg)`;
    };

    window.copyImage = function () {
        const img = document.getElementById("received-img");
        const btn = document.getElementById("copy-btn");
        if (!img.src) return;

        const canvas = document.createElement("canvas");
        const ctx = canvas.getContext("2d");

        if (currentRotation % 180 !== 0) {
            canvas.width = img.naturalHeight;
            canvas.height = img.naturalWidth;
        } else {
            canvas.width = img.naturalWidth;
            canvas.height = img.naturalHeight;
        }

        ctx.translate(canvas.width / 2, canvas.height / 2);
        ctx.rotate(currentRotation * Math.PI / 180);
        ctx.drawImage(img, -img.naturalWidth / 2, -img.naturalHeight / 2);

        canvas.toBlob(async function (blob) {
            try {
                await navigator.clipboard.write([new ClipboardItem({ "image/png": blob })]);
                const originalText = btn.innerText;
                btn.innerText = "已复制";
                btn.style.background = "#28a745";
                setTimeout(() => {
                    btn.innerText = originalText;
                    btn.style.background = "#0a84ff";
                }, 2000);
            } catch (err) {
                alert("复制失败");
            }
        }, "image/png");
    };

    window.openLightbox = function () {
        const src = document.getElementById("received-img").src;
        if (!src) return;
        const lb = document.getElementById("lightbox");
        const lbImg = document.getElementById("lightbox-img");

        lb.style.display = "flex";
        lbImg.src = src;
        scale = 1;
        translateX = 0;
        translateY = 0;
        updateTransform();
        lbImg.style.transform = `rotate(${currentRotation}deg) scale(1)`;
    };

    window.closeLightbox = function () {
        document.getElementById("lightbox").style.display = "none";
    };

    document.getElementById("lightbox").addEventListener("wheel", function (e) {
        e.preventDefault();
        scale *= e.deltaY > 0 ? 0.9 : 1.1;
        if (scale < 0.5) scale = 0.5;
        if (scale > 5) scale = 5;
        updateTransform();
    });

    const lbImg = document.getElementById("lightbox-img");
    lbImg.addEventListener("mousedown", e => {
        isDragging = true;
        startX = e.clientX - translateX;
        startY = e.clientY - translateY;
        lbImg.style.cursor = "grabbing";
    });
    window.addEventListener("mouseup", () => {
        isDragging = false;
        lbImg.style.cursor = "grab";
    });
    window.addEventListener("mousemove", e => {
        if (!isDragging) return;
        e.preventDefault();
        translateX = e.clientX - startX;
        translateY = e.clientY - startY;
        updateTransform();
    });

    function updateTransform() {
        const img = document.getElementById("lightbox-img");
        img.style.transform = `translate(${translateX}px, ${translateY}px) rotate(${currentRotation}deg) scale(${scale})`;
    }

    function initPhone() {
        document.getElementById("phone-area").style.display = "block";
        startCamera();
    }

    async function startCamera() {
        const video = document.getElementById("camera-feed");
        try {
            const stream = await navigator.mediaDevices.getUserMedia({
                video: { facingMode: "environment", width: { ideal: 1920 }, height: { ideal: 1080 } },
                audio: false
            });
            video.srcObject = stream;
            setStatus("upload-status", "准备拍摄", "#4caf50");
        } catch (err) {
            setStatus("upload-status", "请检查相机权限", "#ffcc00");
        }
    }

    window.uploadImage = async function (canvas) {
        const status = document.getElementById("upload-status");
        status.innerText = "处理中...";

        let finalWidth = canvas.width;
        let finalHeight = canvas.height;
        if (canvas.width > MAX_WIDTH) {
            const s = MAX_WIDTH / canvas.width;
            finalWidth = MAX_WIDTH;
            finalHeight = Math.round(canvas.height * s);
        }

        const cCanvas = document.createElement("canvas");
        cCanvas.width = finalWidth;
        cCanvas.height = finalHeight;
        cCanvas.getContext("2d").drawImage(canvas, 0, 0, finalWidth, finalHeight);
        const dataUrl = cCanvas.toDataURL("image/jpeg", 0.9);

        try {
            status.innerText = "上传中...";
            await apiFetch("/api/upload", {
                method: "POST",
                body: JSON.stringify({ image: dataUrl })
            });

            status.innerText = "发送成功";
            status.style.color = "#4caf50";
            document.getElementById("camera-container").style.border = "4px solid #4caf50";
            setTimeout(() => {
                document.getElementById("camera-container").style.border = "none";
            }, 500);
        } catch (error) {
            console.error(error);
            status.innerText = "失败：" + error.message;
            status.style.color = "#ff6b6b";
        }
    };

    window.takePhoto = function () {
        const video = document.getElementById("camera-feed");
        if (!video.srcObject) return;
        const canvas = document.createElement("canvas");
        canvas.width = video.videoWidth;
        canvas.height = video.videoHeight;
        canvas.getContext("2d").drawImage(video, 0, 0);
        uploadImage(canvas);
    };

    window.handleFileSelect = function (event) {
        const file = event.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = function (e) {
            const img = new Image();
            img.src = e.target.result;
            img.onload = function () {
                const canvas = document.createElement("canvas");
                canvas.width = img.width;
                canvas.height = img.height;
                canvas.getContext("2d").drawImage(img, 0, 0);
                uploadImage(canvas);
            };
        };
        reader.readAsDataURL(file);
    };

    init();
})();
