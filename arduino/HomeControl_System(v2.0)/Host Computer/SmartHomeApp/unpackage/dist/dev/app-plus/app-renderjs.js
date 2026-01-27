var __renderjsModules={};

__renderjsModules["246eb7ca"] = (() => {
  var __defProp = Object.defineProperty;
  var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
  var __getOwnPropNames = Object.getOwnPropertyNames;
  var __hasOwnProp = Object.prototype.hasOwnProperty;
  var __export = (target, all) => {
    for (var name in all)
      __defProp(target, name, { get: all[name], enumerable: true });
  };
  var __copyProps = (to, from, except, desc) => {
    if (from && typeof from === "object" || typeof from === "function") {
      for (let key of __getOwnPropNames(from))
        if (!__hasOwnProp.call(to, key) && key !== except)
          __defProp(to, key, { get: () => from[key], enumerable: !(desc = __getOwnPropDesc(from, key)) || desc.enumerable });
    }
    return to;
  };
  var __toCommonJS = (mod) => __copyProps(__defProp({}, "__esModule", { value: true }), mod);
  var __async = (__this, __arguments, generator) => {
    return new Promise((resolve, reject) => {
      var fulfilled = (value) => {
        try {
          step(generator.next(value));
        } catch (e) {
          reject(e);
        }
      };
      var rejected = (value) => {
        try {
          step(generator.throw(value));
        } catch (e) {
          reject(e);
        }
      };
      var step = (x) => x.done ? resolve(x.value) : Promise.resolve(x.value).then(fulfilled, rejected);
      step((generator = generator.apply(__this, __arguments)).next());
    });
  };

  // <stdin>
  var stdin_exports = {};
  __export(stdin_exports, {
    default: () => stdin_default
  });
  var stdin_default = {
    data() {
      return { videoEl: null, stream: null, faceMatcher: null, loopTimer: null, myDescriptor: null, isLoaded: false };
    },
    mounted() {
    },
    beforeDestroy() {
      this.stopCamera();
    },
    methods: {
      receiveCommand(n) {
        if (!n || !n.type)
          return;
        switch (n.type) {
          case "startCam":
            this.startSequence();
            break;
          case "stopCam":
            this.stopCamera();
            break;
          case "register":
            this.registerFace();
            break;
          case "startRec":
            this.startRecognize();
            break;
          case "stopRec":
            this.stopRecognize();
            break;
        }
      },
      startSequence() {
        return __async(this, null, function* () {
          if (this.isLoaded) {
            this.startCamera();
            return;
          }
          this.updateOwner({ loading: true, msg: "\u6B63\u5728\u52A0\u8F7D AI..." });
          if (!window.faceapi)
            yield this.loadScript();
          yield this.initAI();
          this.startCamera();
        });
      },
      loadScript() {
        return new Promise((resolve, reject) => {
          const script = document.createElement("script");
          script.src = "https://cdn.jsdelivr.net/npm/face-api.js@0.22.2/dist/face-api.min.js";
          script.onload = resolve;
          script.onerror = () => {
            this.updateOwner({ loading: false, msg: "\u811A\u672C\u52A0\u8F7D\u5931\u8D25" });
            reject();
          };
          document.head.appendChild(script);
        });
      },
      initAI() {
        return __async(this, null, function* () {
          const faceapi = window.faceapi;
          try {
            const modelUrl = "https://cdn.jsdelivr.net/gh/justadudewhohacks/face-api.js/weights";
            yield Promise.all([
              faceapi.nets.ssdMobilenetv1.loadFromUri(modelUrl),
              faceapi.nets.faceLandmark68Net.loadFromUri(modelUrl),
              faceapi.nets.faceRecognitionNet.loadFromUri(modelUrl)
            ]);
            this.isLoaded = true;
            this.updateOwner({ loading: false, msg: "\u2705 AI \u5C31\u7EEA" });
          } catch (e) {
            this.updateOwner({ loading: false, msg: "\u6A21\u578B\u5931\u8D25" });
          }
        });
      },
      startCamera() {
        return __async(this, null, function* () {
          const container = document.getElementById("video-container");
          if (!this.videoEl) {
            this.videoEl = document.createElement("video");
            this.videoEl.style.cssText = "width:100%;height:100%;object-fit:cover;transform:scaleX(-1);border-radius:12px;";
            this.videoEl.autoplay = true;
            this.videoEl.muted = true;
            this.videoEl.setAttribute("playsinline", "true");
            container.appendChild(this.videoEl);
          }
          try {
            this.stream = yield navigator.mediaDevices.getUserMedia({ video: { facingMode: "user" } });
            this.videoEl.srcObject = this.stream;
            this.updateOwner({ camera: true, msg: "\u{1F4F8} \u8FD0\u884C\u4E2D" });
          } catch (e) {
            this.updateOwner({ msg: "\u65E0\u6444\u50CF\u5934\u6743\u9650" });
          }
        });
      },
      stopCamera() {
        if (this.stream) {
          this.stream.getTracks().forEach((t) => t.stop());
          this.videoEl.srcObject = null;
        }
        this.updateOwner({ camera: false, recognizing: false, msg: "" });
        this.stopRecognize();
      },
      registerFace() {
        return __async(this, null, function* () {
          if (!this.videoEl || this.videoEl.paused)
            return;
          const faceapi = window.faceapi;
          const detection = yield faceapi.detectSingleFace(this.videoEl).withFaceLandmarks().withFaceDescriptor();
          if (detection) {
            this.myDescriptor = detection.descriptor;
            this.faceMatcher = new faceapi.FaceMatcher(this.myDescriptor, 0.6);
            this.updateOwner({ msg: "\u2705 \u5DF2\u5F55\u5165" });
          } else {
            this.updateOwner({ msg: "\u26A0\uFE0F \u672A\u68C0\u6D4B\u5230\u4EBA\u8138" });
          }
        });
      },
      startRecognize() {
        if (!this.myDescriptor)
          return alert("\u8BF7\u5148\u5F55\u5165!");
        this.updateOwner({ recognizing: true, msg: "\u{1F440} \u8BC6\u522B\u4E2D..." });
        const faceapi = window.faceapi;
        if (this.loopTimer)
          clearInterval(this.loopTimer);
        this.loopTimer = setInterval(() => __async(this, null, function* () {
          if (!this.videoEl || this.videoEl.paused)
            return;
          const detection = yield faceapi.detectSingleFace(this.videoEl).withFaceLandmarks().withFaceDescriptor();
          if (detection) {
            const match = this.faceMatcher.findBestMatch(detection.descriptor);
            if (match.label !== "unknown" && match.distance < 0.5) {
              if (this.$ownerInstance)
                this.$ownerInstance.callMethod("onFaceMatch");
              this.updateOwner({ msg: "\u{1F513} \u901A\u8FC7" });
            }
          }
        }), 800);
      },
      stopRecognize() {
        if (this.loopTimer)
          clearInterval(this.loopTimer);
        this.updateOwner({ recognizing: false });
      },
      updateOwner(statusObj) {
        if (this.$ownerInstance)
          this.$ownerInstance.callMethod("onAiStatus", statusObj);
      }
    }
  };
  return __toCommonJS(stdin_exports);
})();
