import argparse
import os
import random
import urllib.request
from pathlib import Path
from time import perf_counter

# Keep Keras model downloads and caches away from the system drive.
os.environ.setdefault("KERAS_HOME", r"D:\keras-cache")
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "1")

import numpy as np
import tensorflow as tf
from PIL import Image, ImageDraw


DEFAULT_STYLE_URL = (
    "https://storage.googleapis.com/download.tensorflow.org/example_images/"
    "Vassily_Kandinsky%2C_1913_-_Composition_7.jpg"
)
DEFAULT_CONTENT_URL = (
    "https://storage.googleapis.com/download.tensorflow.org/example_images/"
    "YellowLabradorLooking_new.jpg"
)
DEFAULT_HUB_MODEL_URL = "https://tfhub.dev/google/magenta/arbitrary-image-stylization-v1-256/2"

STYLE_PRESETS = {
    "balanced": {
        "content_weight": 2.5e-8,
        "style_weight": 1.0e-6,
        "tv_weight": 1.0e-6,
        "learning_rate": 75.0,
        "description": "Keeps the content recognizable while adding clear brush texture.",
    },
    "strong": {
        "content_weight": 1.2e-8,
        "style_weight": 2.5e-6,
        "tv_weight": 1.0e-6,
        "learning_rate": 80.0,
        "description": "More visible style texture, useful for classroom screenshots.",
    },
    "clean": {
        "content_weight": 4.0e-8,
        "style_weight": 7.0e-7,
        "tv_weight": 1.5e-6,
        "learning_rate": 60.0,
        "description": "Gentler style transfer with less noisy detail.",
    },
}

CONTENT_LAYER = "block5_conv2"
STYLE_LAYERS = [
    "block1_conv1",
    "block2_conv1",
    "block3_conv1",
    "block4_conv1",
    "block5_conv1",
]


def default_content_path() -> Path:
    return Path(__file__).resolve().parent / "教材行文代码" / "birdnest.jpg"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "TensorFlow neural style transfer: combine the content of one image "
            "with the artistic texture of another image using a pretrained VGG19."
        )
    )
    parser.add_argument(
        "--method",
        choices=["hub", "optimize"],
        default="hub",
        help=(
            "hub uses a pretrained TensorFlow Hub stylization model for fast, stable results; "
            "optimize runs the hand-written VGG19 loss optimization loop."
        ),
    )
    parser.add_argument(
        "--hub-model-url",
        type=str,
        default=DEFAULT_HUB_MODEL_URL,
        help="TensorFlow Hub arbitrary image stylization model URL used by --method hub.",
    )
    parser.add_argument(
        "--content-image",
        type=str,
        default=str(default_content_path()),
        help="Content image path. Defaults to the course birdnest.jpg image.",
    )
    parser.add_argument(
        "--demo-content",
        action="store_true",
        help="Download and use TensorFlow's public Labrador demo photo as the content image.",
    )
    parser.add_argument(
        "--content-url",
        type=str,
        default=DEFAULT_CONTENT_URL,
        help="URL used when --demo-content is enabled.",
    )
    parser.add_argument(
        "--style-image",
        type=str,
        default="",
        help="Style image path. If omitted, a public Starry Night style image is downloaded.",
    )
    parser.add_argument(
        "--style-url",
        type=str,
        default=DEFAULT_STYLE_URL,
        help="URL used when --style-image is omitted.",
    )
    parser.add_argument(
        "--preset",
        choices=sorted(STYLE_PRESETS),
        default="balanced",
        help="Weight preset for content/style/smoothness losses.",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=350,
        help="Optimization iterations. More iterations usually improve style quality.",
    )
    parser.add_argument(
        "--max-size",
        type=int,
        default=512,
        help="Longest side of the working image. Larger is sharper but slower.",
    )
    parser.add_argument(
        "--learning-rate",
        type=float,
        default=None,
        help="Override the preset optimizer learning rate.",
    )
    parser.add_argument(
        "--content-weight",
        type=float,
        default=None,
        help="Override content loss weight.",
    )
    parser.add_argument(
        "--style-weight",
        type=float,
        default=None,
        help="Override style loss weight.",
    )
    parser.add_argument(
        "--tv-weight",
        type=float,
        default=None,
        help="Override total variation loss weight for image smoothness.",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default=str(Path("outputs") / "neural_style_transfer"),
        help="Directory for generated images and run report.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=123,
        help="Random seed used for deterministic initialization.",
    )
    parser.add_argument(
        "--init",
        choices=["content", "noise"],
        default="content",
        help="Initialize the generated image from the content image or noisy pixels.",
    )
    parser.add_argument(
        "--save-every",
        type=int,
        default=100,
        help="Save progress images every N iterations. Use 0 to disable progress images.",
    )
    return parser.parse_args()


def prepare_runtime(seed: int) -> list[str]:
    random.seed(seed)
    np.random.seed(seed)
    tf.random.set_seed(seed)
    return [f"{device.device_type}: {device.name}" for device in tf.config.list_physical_devices()]


def resolve_path(path_text: str) -> Path:
    path = Path(path_text)
    if not path.is_absolute():
        path = Path.cwd() / path
    return path.resolve()


def ensure_output_dir(path_text: str) -> Path:
    output_dir = resolve_path(path_text)
    output_dir.mkdir(parents=True, exist_ok=True)
    return output_dir


def download_default_style(style_url: str, output_dir: Path) -> Path:
    style_path = output_dir / "style_reference_kandinsky.jpg"
    if style_path.exists() and style_path.stat().st_size > 0:
        return style_path

    print(f"Downloading default style image: {style_url}")
    try:
        with urllib.request.urlopen(style_url, timeout=60) as response:
            data = response.read()
        style_path.write_bytes(data)
    except Exception as exc:
        print(f"Download failed ({exc}). Generating a local starry-brush style image instead.")
        generate_fallback_style_image(style_path)
    return style_path


def download_demo_content(content_url: str, output_dir: Path) -> Path:
    content_path = output_dir / "content_demo_labrador.jpg"
    if content_path.exists() and content_path.stat().st_size > 0:
        return content_path

    print(f"Downloading demo content image: {content_url}")
    try:
        with urllib.request.urlopen(content_url, timeout=60) as response:
            data = response.read()
        content_path.write_bytes(data)
    except Exception as exc:
        cached_path = (
            Path(__file__).resolve().parent
            / "outputs"
            / "neural_style_transfer_demo_labrador"
            / "content_demo_labrador.jpg"
        )
        if cached_path.exists() and cached_path.stat().st_size > 0:
            print(f"Download failed ({exc}). Reusing cached demo content: {cached_path}")
            return cached_path
        fallback_path = default_content_path()
        if fallback_path.exists():
            print(f"Download failed ({exc}). Falling back to course content image: {fallback_path}")
            return fallback_path
        raise
    return content_path


def generate_fallback_style_image(path: Path, size: tuple[int, int] = (768, 512)) -> None:
    width, height = size
    image = Image.new("RGB", size, (12, 24, 58))
    draw = ImageDraw.Draw(image, "RGBA")
    rng = random.Random(20260707)

    for y in range(height):
        blue = int(46 + 42 * y / height)
        green = int(26 + 20 * y / height)
        draw.line([(0, y), (width, y)], fill=(10, green, blue, 255))

    for _ in range(130):
        x = rng.randint(-80, width + 80)
        y = rng.randint(-40, height + 40)
        span = rng.randint(80, 220)
        color = rng.choice(
            [
                (33, 95, 176, 90),
                (58, 145, 211, 75),
                (242, 193, 72, 110),
                (249, 230, 136, 100),
                (235, 103, 66, 65),
                (255, 255, 235, 90),
            ]
        )
        width_px = rng.randint(5, 18)
        points = []
        for step in range(12):
            px = x + step * span / 11
            py = y + rng.randint(-45, 45)
            points.append((px, py))
        draw.line(points, fill=color, width=width_px, joint="curve")

    for _ in range(45):
        x = rng.randint(0, width)
        y = rng.randint(0, height)
        radius = rng.randint(8, 34)
        fill = rng.choice([(255, 234, 112, 145), (255, 248, 205, 165), (83, 174, 229, 95)])
        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=fill)

    image.save(path)


def get_image_size(path: Path, max_size: int) -> tuple[int, int]:
    with Image.open(path) as image:
        width, height = image.convert("RGB").size
    scale = max_size / max(width, height)
    if scale >= 1:
        return width, height
    return int(width * scale), int(height * scale)


def load_image(path: Path, target_size: tuple[int, int]) -> tf.Tensor:
    image = Image.open(path).convert("RGB").resize(target_size, Image.Resampling.LANCZOS)
    array = np.asarray(image).astype("float32")
    return tf.convert_to_tensor(array[None, ...])


def save_tensor_image(image_tensor: tf.Tensor | np.ndarray, path: Path) -> None:
    array = tensor_to_uint8_array(image_tensor)
    Image.fromarray(array).save(path)


def tensor_to_uint8_array(image_tensor: tf.Tensor | np.ndarray) -> np.ndarray:
    array = np.asarray(image_tensor)
    if array.ndim == 4:
        array = array[0]
    if np.issubdtype(array.dtype, np.floating) and np.nanmax(array) <= 1.5:
        array = array * 255.0
    array = np.clip(array, 0, 255).astype("uint8")
    return array


def build_feature_extractor() -> tf.keras.Model:
    base_model = tf.keras.applications.VGG19(weights="imagenet", include_top=False)
    base_model.trainable = False
    outputs = [base_model.get_layer(name).output for name in [CONTENT_LAYER, *STYLE_LAYERS]]
    return tf.keras.Model(inputs=base_model.input, outputs=outputs)


def preprocess_for_vgg(image_tensor: tf.Tensor) -> tf.Tensor:
    return tf.keras.applications.vgg19.preprocess_input(image_tensor)


def gram_matrix(features: tf.Tensor) -> tf.Tensor:
    features = tf.squeeze(features, axis=0)
    gram = tf.linalg.einsum("hwd,hwe->de", features, features)
    height = tf.shape(features)[0]
    width = tf.shape(features)[1]
    channels = tf.shape(features)[2]
    return gram / tf.cast(height * width * channels, tf.float32)


def extract_targets(
    extractor: tf.keras.Model, content_image: tf.Tensor, style_image: tf.Tensor
) -> tuple[tf.Tensor, list[tf.Tensor]]:
    content_outputs = extractor(preprocess_for_vgg(content_image))
    style_outputs = extractor(preprocess_for_vgg(style_image))
    content_target = content_outputs[0]
    style_targets = [gram_matrix(style_output) for style_output in style_outputs[1:]]
    return content_target, style_targets


def calculate_losses(
    extractor: tf.keras.Model,
    generated_image: tf.Tensor,
    content_target: tf.Tensor,
    style_targets: list[tf.Tensor],
    content_weight: float,
    style_weight: float,
    tv_weight: float,
) -> tuple[tf.Tensor, tf.Tensor, tf.Tensor, tf.Tensor]:
    outputs = extractor(preprocess_for_vgg(generated_image))
    generated_content = outputs[0]
    generated_styles = outputs[1:]

    content_loss = tf.reduce_mean(tf.square(generated_content - content_target))

    style_loss = tf.constant(0.0, dtype=tf.float32)
    for generated_features, style_target in zip(generated_styles, style_targets):
        style_loss += tf.reduce_mean(tf.square(gram_matrix(generated_features) - style_target))
    style_loss /= len(style_targets)

    tv_loss = tf.reduce_mean(tf.image.total_variation(generated_image))
    total_loss = content_weight * content_loss + style_weight * style_loss + tv_weight * tv_loss
    return total_loss, content_loss, style_loss, tv_loss


@tf.function
def train_step(
    extractor: tf.keras.Model,
    generated_image: tf.Variable,
    optimizer: tf.keras.optimizers.Optimizer,
    content_target: tf.Tensor,
    style_targets: list[tf.Tensor],
    content_weight: float,
    style_weight: float,
    tv_weight: float,
) -> tuple[tf.Tensor, tf.Tensor, tf.Tensor, tf.Tensor]:
    with tf.GradientTape() as tape:
        total_loss, content_loss, style_loss, tv_loss = calculate_losses(
            extractor=extractor,
            generated_image=generated_image,
            content_target=content_target,
            style_targets=style_targets,
            content_weight=content_weight,
            style_weight=style_weight,
            tv_weight=tv_weight,
        )
    gradients = tape.gradient(total_loss, generated_image)
    optimizer.apply_gradients([(gradients, generated_image)])
    generated_image.assign(tf.clip_by_value(generated_image, 0.0, 255.0))
    return total_loss, content_loss, style_loss, tv_loss


def initialize_generated_image(content_image: tf.Tensor, init_mode: str) -> tf.Variable:
    if init_mode == "content":
        return tf.Variable(content_image)

    noise = tf.random.uniform(tf.shape(content_image), minval=0.0, maxval=255.0)
    mixed = content_image * 0.65 + noise * 0.35
    return tf.Variable(tf.clip_by_value(mixed, 0.0, 255.0))


def make_comparison_grid(
    content_image: tf.Tensor,
    style_image: tf.Tensor,
    result_image: tf.Tensor,
    output_dir: Path,
) -> None:
    labels = [
        ("Content", content_image),
        ("Style", style_image),
        ("Generated", result_image),
    ]
    width = int(content_image.shape[2])
    height = int(content_image.shape[1])
    title_height = 34
    canvas = Image.new("RGB", (width * len(labels), height + title_height), "white")
    draw = ImageDraw.Draw(canvas)

    for index, (label, tensor) in enumerate(labels):
        array = tensor_to_uint8_array(tensor)
        tile = Image.fromarray(array).resize((width, height), Image.Resampling.LANCZOS)
        x = index * width
        canvas.paste(tile, (x, title_height))
        draw.text((x + 12, 10), label, fill=(20, 20, 20))

    canvas.save(output_dir / "comparison_grid.png")


def run_hub_style_transfer(
    content_image: tf.Tensor,
    style_image: tf.Tensor,
    hub_model_url: str,
) -> tf.Tensor:
    try:
        import tensorflow_hub as hub
    except ImportError as exc:
        raise ImportError(
            "The hub method needs tensorflow-hub. Install it with: "
            "python -m pip install tensorflow-hub"
        ) from exc

    cache_dir = (Path.home() / "tfhub_cache").as_posix()
    os.environ.setdefault("TFHUB_CACHE_DIR", cache_dir)
    Path(cache_dir).mkdir(parents=True, exist_ok=True)

    print(f"Loading TensorFlow Hub model: {hub_model_url}")
    model = hub.load(hub_model_url)
    print("Running pretrained style transfer model")
    return model(content_image / 255.0, style_image / 255.0)[0]


def write_hub_report(
    output_dir: Path,
    content_path: Path,
    style_path: Path,
    hub_model_url: str,
    max_size: int,
    elapsed_seconds: float,
    devices: list[str],
) -> None:
    lines = [
        "TensorFlow Hub Neural Style Transfer Run Report",
        "================================================",
        f"TensorFlow version: {tf.__version__}",
        f"Keras home: {os.environ.get('KERAS_HOME')}",
        f"TensorFlow Hub cache: {os.environ.get('TFHUB_CACHE_DIR')}",
        f"Content image: {content_path}",
        f"Style image: {style_path}",
        f"Hub model: {hub_model_url}",
        f"Output directory: {output_dir}",
        f"Max size: {max_size}",
        f"Elapsed seconds: {elapsed_seconds:.2f}",
        "",
        "Devices:",
    ]
    lines.extend(f"- {device}" for device in devices)
    lines.extend(
        [
            "",
            "Generated files:",
            "- content.png",
            "- style_reference.png",
            "- stylized_result.png",
            "- comparison_grid.png",
            "- run_report.txt",
            "",
            "Presentation summary:",
            "This mode uses a pretrained TensorFlow Hub arbitrary image stylization network.",
            "The model has already learned how to transfer visual texture from a style image.",
            "At runtime the program only performs inference: one content image plus one style image produces a stylized output image.",
            "The same file also contains an optimize mode that shows the VGG19 content/style loss idea step by step.",
        ]
    )
    (output_dir / "run_report.txt").write_text("\n".join(lines), encoding="utf-8")


def write_report(
    output_dir: Path,
    content_path: Path,
    style_path: Path,
    preset_name: str,
    weights: dict[str, float],
    iterations: int,
    max_size: int,
    init_mode: str,
    elapsed_seconds: float,
    devices: list[str],
    losses: dict[str, float],
) -> None:
    lines = [
        "TensorFlow Neural Style Transfer Run Report",
        "============================================",
        f"TensorFlow version: {tf.__version__}",
        f"Keras home: {os.environ.get('KERAS_HOME')}",
        f"Content image: {content_path}",
        f"Style image: {style_path}",
        f"Output directory: {output_dir}",
        f"Preset: {preset_name}",
        f"Preset description: {STYLE_PRESETS[preset_name]['description']}",
        f"Iterations: {iterations}",
        f"Max size: {max_size}",
        f"Initialization: {init_mode}",
        f"Learning rate: {weights['learning_rate']}",
        f"Content weight: {weights['content_weight']}",
        f"Style weight: {weights['style_weight']}",
        f"Total variation weight: {weights['tv_weight']}",
        f"Elapsed seconds: {elapsed_seconds:.2f}",
        "",
        "Devices:",
    ]
    lines.extend(f"- {device}" for device in devices)
    lines.extend(
        [
            "",
            "Final losses:",
            f"- total: {losses['total']:.4f}",
            f"- content: {losses['content']:.4f}",
            f"- style: {losses['style']:.4f}",
            f"- total variation: {losses['tv']:.4f}",
            "",
            "Generated files:",
            "- content.png",
            "- style_reference.png",
            "- stylized_result.png",
            "- comparison_grid.png",
            "- run_report.txt",
            "",
            "Presentation summary:",
            "Neural style transfer freezes a pretrained VGG19 network. It does not train the model weights.",
            "Instead, it optimizes the output image so deep features stay close to the content photo,",
            "while Gram matrices of several convolution layers match the texture statistics of the style image.",
        ]
    )
    (output_dir / "run_report.txt").write_text("\n".join(lines), encoding="utf-8")


def get_effective_weights(args: argparse.Namespace) -> dict[str, float]:
    preset = STYLE_PRESETS[args.preset]
    return {
        "content_weight": args.content_weight
        if args.content_weight is not None
        else preset["content_weight"],
        "style_weight": args.style_weight if args.style_weight is not None else preset["style_weight"],
        "tv_weight": args.tv_weight if args.tv_weight is not None else preset["tv_weight"],
        "learning_rate": args.learning_rate
        if args.learning_rate is not None
        else preset["learning_rate"],
    }


def main() -> None:
    args = parse_args()
    if args.iterations < 1:
        raise ValueError("--iterations must be at least 1")
    if args.max_size < 128:
        raise ValueError("--max-size must be at least 128")

    devices = prepare_runtime(args.seed)
    output_dir = ensure_output_dir(args.output_dir)
    content_path = (
        download_demo_content(args.content_url, output_dir)
        if args.demo_content
        else resolve_path(args.content_image)
    )
    if not content_path.exists():
        raise FileNotFoundError(f"Content image not found: {content_path}")

    style_path = resolve_path(args.style_image) if args.style_image else download_default_style(args.style_url, output_dir)
    if not style_path.exists():
        raise FileNotFoundError(f"Style image not found: {style_path}")

    target_size = get_image_size(content_path, args.max_size)
    content_image = load_image(content_path, target_size)
    style_image = load_image(style_path, target_size)

    print("TensorFlow Neural Style Transfer")
    print(f"TensorFlow: {tf.__version__}")
    print(f"Method: {args.method}")
    print(f"Devices: {', '.join(devices) if devices else 'none reported'}")
    print(f"Content: {content_path}")
    print(f"Style: {style_path}")
    print(f"Working size: {target_size[0]}x{target_size[1]}")
    print(f"Output dir: {output_dir}")

    save_tensor_image(content_image, output_dir / "content.png")
    save_tensor_image(style_image, output_dir / "style_reference.png")

    if args.method == "hub":
        start = perf_counter()
        stylized_image = run_hub_style_transfer(
            content_image=content_image,
            style_image=style_image,
            hub_model_url=args.hub_model_url,
        )
        elapsed_seconds = perf_counter() - start
        save_tensor_image(stylized_image, output_dir / "stylized_result.png")
        make_comparison_grid(content_image, style_image, stylized_image, output_dir)
        write_hub_report(
            output_dir=output_dir,
            content_path=content_path,
            style_path=style_path,
            hub_model_url=args.hub_model_url,
            max_size=args.max_size,
            elapsed_seconds=elapsed_seconds,
            devices=devices,
        )

        print(f"Saved result: {output_dir / 'stylized_result.png'}")
        print(f"Saved comparison: {output_dir / 'comparison_grid.png'}")
        print(f"Saved report: {output_dir / 'run_report.txt'}")
        return

    weights = get_effective_weights(args)
    print(f"Preset: {args.preset} ({STYLE_PRESETS[args.preset]['description']})")

    extractor = build_feature_extractor()
    content_target, style_targets = extract_targets(extractor, content_image, style_image)
    generated_image = initialize_generated_image(content_image, args.init)
    optimizer = tf.keras.optimizers.Adam(learning_rate=weights["learning_rate"])

    start = perf_counter()
    final_losses = {}
    for iteration in range(1, args.iterations + 1):
        total_loss, content_loss, style_loss, tv_loss = train_step(
            extractor=extractor,
            generated_image=generated_image,
            optimizer=optimizer,
            content_target=content_target,
            style_targets=style_targets,
            content_weight=weights["content_weight"],
            style_weight=weights["style_weight"],
            tv_weight=weights["tv_weight"],
        )
        final_losses = {
            "total": float(total_loss.numpy()),
            "content": float(content_loss.numpy()),
            "style": float(style_loss.numpy()),
            "tv": float(tv_loss.numpy()),
        }

        if iteration == 1 or iteration % 25 == 0 or iteration == args.iterations:
            print(
                f"Iteration {iteration:4d}/{args.iterations}: "
                f"total={final_losses['total']:.4f}, "
                f"content={final_losses['content']:.2f}, "
                f"style={final_losses['style']:.2f}, "
                f"tv={final_losses['tv']:.2f}"
            )

        if args.save_every and iteration % args.save_every == 0 and iteration != args.iterations:
            save_tensor_image(generated_image, output_dir / f"progress_{iteration:04d}.png")

    elapsed_seconds = perf_counter() - start
    save_tensor_image(generated_image, output_dir / "stylized_result.png")
    make_comparison_grid(content_image, style_image, generated_image, output_dir)
    write_report(
        output_dir=output_dir,
        content_path=content_path,
        style_path=style_path,
        preset_name=args.preset,
        weights=weights,
        iterations=args.iterations,
        max_size=args.max_size,
        init_mode=args.init,
        elapsed_seconds=elapsed_seconds,
        devices=devices,
        losses=final_losses,
    )

    print(f"Saved result: {output_dir / 'stylized_result.png'}")
    print(f"Saved comparison: {output_dir / 'comparison_grid.png'}")
    print(f"Saved report: {output_dir / 'run_report.txt'}")


if __name__ == "__main__":
    main()
