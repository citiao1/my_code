import argparse
import os
import tempfile
from pathlib import Path
from time import perf_counter

PROJECT_DIR = Path(__file__).resolve().parent


def default_model_cache_dir() -> Path:
    for candidate in [
        PROJECT_DIR / ".model-cache",
        Path.home() / ".software_practice_model_cache",
        Path(tempfile.gettempdir()) / "software_practice_model_cache",
    ]:
        if candidate.as_posix().isascii():
            return candidate
    return PROJECT_DIR / ".model-cache"


# Keep model downloads out of machine-specific hard-coded drives. TensorFlow Hub
# is happier when the cache path is ASCII, so non-ASCII project paths fall back.
MODEL_CACHE_DIR = default_model_cache_dir()
os.environ.setdefault("KERAS_HOME", str(MODEL_CACHE_DIR / "keras"))
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "1")

import numpy as np
import tensorflow as tf
from PIL import Image, ImageDraw


PRESETS = {
    "soft": {
        "description": "Light texture enhancement with gentle low-level patterns.",
        "layers": {
            "mixed2": 0.7,
            "mixed3": 0.3,
        },
        "step_scale": 0.75,
    },
    "classic": {
        "description": "Balanced DeepDream look with recognizable neural patterns.",
        "layers": {
            "mixed3": 0.4,
            "mixed5": 0.9,
            "mixed7": 1.2,
        },
        "step_scale": 1.0,
    },
    "wild": {
        "description": "Stronger hallucination effect from deeper Inception features.",
        "layers": {
            "mixed5": 0.8,
            "mixed7": 1.2,
            "mixed9": 1.8,
        },
        "step_scale": 1.25,
    },
}


def default_image_path() -> Path:
    return Path(__file__).resolve().parent / "教材行文代码" / "birdnest.jpg"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "TensorFlow DeepDream camera: optimize a normal photo so that "
            "InceptionV3 hidden features become visually visible."
        )
    )
    parser.add_argument(
        "--image",
        type=str,
        default=str(default_image_path()),
        help="Input image path. Defaults to the course birdnest.jpg image.",
    )
    parser.add_argument(
        "--preset",
        choices=["all", "soft", "classic", "wild"],
        default="all",
        help="Dream style to generate. Use all to create all three presets.",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=20,
        help="Gradient ascent iterations per octave.",
    )
    parser.add_argument(
        "--octaves",
        type=int,
        default=3,
        help="Number of image scales. More octaves preserve large and small features.",
    )
    parser.add_argument(
        "--octave-scale",
        type=float,
        default=1.4,
        help="Scale factor between adjacent octaves.",
    )
    parser.add_argument(
        "--step",
        type=float,
        default=0.01,
        help="Base gradient ascent step size.",
    )
    parser.add_argument(
        "--max-size",
        type=int,
        default=768,
        help="Longest side of the working image.",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default=str(Path("outputs") / "deepdream_camera"),
        help="Directory for generated images and run report.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for deterministic TensorFlow operations where possible.",
    )
    return parser.parse_args()


def prepare_runtime(seed: int) -> list[str]:
    np.random.seed(seed)
    tf.random.set_seed(seed)
    devices = []
    for device_type in ("CPU", "GPU"):
        for device in tf.config.list_physical_devices(device_type):
            devices.append(f"{device_type}: {device.name}")
    return devices


def resolve_path(path_text: str) -> Path:
    path = Path(path_text)
    if not path.is_absolute():
        path = Path.cwd() / path
    return path.resolve()


def ensure_output_dir(path_text: str) -> Path:
    output_dir = resolve_path(path_text)
    output_dir.mkdir(parents=True, exist_ok=True)
    return output_dir


def resize_keep_aspect(image: Image.Image, max_size: int) -> Image.Image:
    width, height = image.size
    longest = max(width, height)
    if longest <= max_size:
        return image
    scale = max_size / float(longest)
    new_size = (max(96, int(width * scale)), max(96, int(height * scale)))
    return image.resize(new_size, Image.Resampling.LANCZOS)


def load_image_pixels(path: Path, max_size: int) -> tuple[np.ndarray, tuple[int, int]]:
    if not path.exists():
        raise FileNotFoundError(f"Input image not found: {path}")
    image = Image.open(path).convert("RGB")
    image = resize_keep_aspect(image, max_size)
    width, height = image.size
    pixels = np.asarray(image).astype("float32")
    return pixels, (width, height)


def preprocess_pixels(pixels: np.ndarray) -> tf.Tensor:
    batch = np.expand_dims(pixels.copy(), axis=0)
    return tf.keras.applications.inception_v3.preprocess_input(batch)


def deprocess_tensor(image_tensor: tf.Tensor | np.ndarray) -> np.ndarray:
    image = np.array(image_tensor)
    if image.ndim == 4:
        image = image[0]
    image = 127.5 * (image + 1.0)
    image = np.clip(image, 0, 255).astype("uint8")
    return image


def save_array_image(array: np.ndarray, path: Path) -> None:
    Image.fromarray(array).save(path)


def get_requested_presets(preset: str) -> list[str]:
    if preset == "all":
        return ["soft", "classic", "wild"]
    return [preset]


def build_feature_model(preset_names: list[str]) -> tf.keras.Model:
    all_layer_names = []
    for preset_name in preset_names:
        for layer_name in PRESETS[preset_name]["layers"]:
            if layer_name not in all_layer_names:
                all_layer_names.append(layer_name)

    base_model = tf.keras.applications.InceptionV3(
        weights="imagenet",
        include_top=False,
    )
    base_model.trainable = False
    outputs = [base_model.get_layer(layer_name).output for layer_name in all_layer_names]
    feature_model = tf.keras.Model(inputs=base_model.input, outputs=outputs)
    feature_model.trainable = False
    feature_model.layer_names_for_dream = all_layer_names
    return feature_model


def calculate_dream_loss(
    dream_image: tf.Tensor,
    feature_model: tf.keras.Model,
    layer_weights: dict[str, float],
) -> tf.Tensor:
    activations = feature_model(dream_image)
    if not isinstance(activations, list):
        activations = [activations]

    loss = tf.constant(0.0, dtype=tf.float32)
    active_layers = set(layer_weights)
    for layer_name, activation in zip(feature_model.layer_names_for_dream, activations):
        if layer_name not in active_layers:
            continue
        weight = tf.cast(layer_weights[layer_name], tf.float32)
        # Border pixels tend to create frame artifacts, so focus on inner features.
        if activation.shape[1] is not None and activation.shape[1] > 4:
            activation = activation[:, 2:-2, 2:-2, :]
        layer_loss = tf.reduce_mean(tf.square(activation))
        loss = loss + weight * layer_loss
    return loss


def gradient_ascent_step(
    dream_image: tf.Tensor,
    feature_model: tf.keras.Model,
    layer_weights: dict[str, float],
    step_size: float,
) -> tuple[tf.Tensor, float]:
    with tf.GradientTape() as tape:
        tape.watch(dream_image)
        loss = calculate_dream_loss(dream_image, feature_model, layer_weights)

    gradients = tape.gradient(loss, dream_image)
    gradients = gradients / (tf.math.reduce_std(gradients) + 1e-8)
    dream_image = dream_image + gradients * step_size
    dream_image = tf.clip_by_value(dream_image, -1.0, 1.0)
    return dream_image, float(loss.numpy())


def build_octave_shapes(base_shape: tuple[int, int], octaves: int, octave_scale: float) -> list[tuple[int, int]]:
    height, width = base_shape
    shapes = []
    for octave_index in range(octaves):
        scale = octave_scale ** (octaves - octave_index - 1)
        scaled_height = max(96, int(round(height / scale)))
        scaled_width = max(96, int(round(width / scale)))
        shapes.append((scaled_height, scaled_width))
    shapes[-1] = (height, width)
    return shapes


def resize_tensor(dream_image: tf.Tensor, shape: tuple[int, int]) -> tf.Tensor:
    resized = tf.image.resize(dream_image, shape, method="bicubic")
    return tf.clip_by_value(resized, -1.0, 1.0)


def run_deepdream(
    source_image: tf.Tensor,
    preset_name: str,
    feature_model: tf.keras.Model,
    iterations: int,
    octaves: int,
    octave_scale: float,
    step: float,
) -> dict[str, object]:
    preset = PRESETS[preset_name]
    layer_weights = preset["layers"]
    step_size = step * preset["step_scale"]
    original_height = int(source_image.shape[1])
    original_width = int(source_image.shape[2])
    octave_shapes = build_octave_shapes((original_height, original_width), octaves, octave_scale)

    dream_image = resize_tensor(source_image, octave_shapes[0])
    losses: list[float] = []
    progress_images: list[np.ndarray] = []

    print(f"\nPreset: {preset_name}")
    print(f"Layers: {', '.join(layer_weights.keys())}")
    for octave_number, shape in enumerate(octave_shapes, start=1):
        dream_image = resize_tensor(dream_image, shape)
        print(f"  Octave {octave_number}/{len(octave_shapes)} shape={shape}")
        for iteration in range(1, iterations + 1):
            dream_image, loss_value = gradient_ascent_step(
                dream_image=dream_image,
                feature_model=feature_model,
                layer_weights=layer_weights,
                step_size=step_size,
            )
            losses.append(loss_value)
            if iteration == 1 or iteration == iterations or iteration % max(1, iterations // 4) == 0:
                print(f"    iter {iteration:02d}/{iterations} loss={loss_value:.4f}")
        progress_images.append(deprocess_tensor(dream_image))

    final_image = deprocess_tensor(dream_image)
    return {
        "preset": preset_name,
        "final": final_image,
        "progress": progress_images,
        "losses": losses,
        "layers": layer_weights,
        "description": preset["description"],
    }


def fit_tile(image: Image.Image, tile_size: tuple[int, int]) -> Image.Image:
    tile_width, tile_height = tile_size
    working = image.copy()
    working.thumbnail((tile_width, tile_height - 28), Image.Resampling.LANCZOS)
    canvas = Image.new("RGB", tile_size, "white")
    x = (tile_width - working.width) // 2
    y = 26 + (tile_height - 28 - working.height) // 2
    canvas.paste(working, (x, y))
    return canvas


def draw_title(canvas: Image.Image, title: str) -> None:
    draw = ImageDraw.Draw(canvas)
    draw.rectangle((0, 0, canvas.width, 25), fill=(30, 30, 30))
    draw.text((8, 6), title, fill=(255, 255, 255))


def make_grid(items: list[tuple[str, np.ndarray]], columns: int, tile_size: tuple[int, int]) -> Image.Image:
    rows = int(np.ceil(len(items) / columns))
    grid = Image.new("RGB", (columns * tile_size[0], rows * tile_size[1]), "white")
    for index, (title, array) in enumerate(items):
        tile = fit_tile(Image.fromarray(array).convert("RGB"), tile_size)
        draw_title(tile, title)
        x = (index % columns) * tile_size[0]
        y = (index // columns) * tile_size[1]
        grid.paste(tile, (x, y))
    return grid


def save_comparison_grid(original: np.ndarray, results: dict[str, dict[str, object]], output_dir: Path) -> None:
    items = [("original", original)]
    for preset_name, result in results.items():
        items.append((f"dream_{preset_name}", result["final"]))
    grid = make_grid(items, columns=2, tile_size=(360, 300))
    grid.save(output_dir / "comparison_grid.png")


def save_progress_grid(result: dict[str, object], output_dir: Path) -> None:
    preset_name = str(result["preset"])
    progress = result["progress"]
    items = [(f"{preset_name}_octave_{index + 1}", array) for index, array in enumerate(progress)]
    grid = make_grid(items, columns=min(3, len(items)), tile_size=(320, 260))
    grid.save(output_dir / f"octave_progress_{preset_name}.png")
    if preset_name == "classic":
        grid.save(output_dir / "octave_progress_classic.png")


def write_report(
    output_dir: Path,
    args: argparse.Namespace,
    image_path: Path,
    image_size: tuple[int, int],
    devices: list[str],
    results: dict[str, dict[str, object]],
    elapsed_seconds: float,
) -> None:
    lines = [
        "TensorFlow DeepDream Camera Run Report",
        "=" * 42,
        f"TensorFlow version: {tf.__version__}",
        f"Keras home: {os.environ.get('KERAS_HOME')}",
        f"Input image: {image_path}",
        f"Working image size: {image_size[0]}x{image_size[1]}",
        f"Output directory: {output_dir}",
        f"Preset argument: {args.preset}",
        f"Iterations per octave: {args.iterations}",
        f"Octaves: {args.octaves}",
        f"Octave scale: {args.octave_scale}",
        f"Step size: {args.step}",
        f"Elapsed seconds: {elapsed_seconds:.2f}",
        "",
        "Devices:",
    ]
    lines.extend(f"- {device}" for device in devices)
    lines.append("")
    lines.append("Preset results:")
    for preset_name, result in results.items():
        losses = result["losses"]
        lines.append(f"- {preset_name}:")
        lines.append(f"  description: {result['description']}")
        lines.append(f"  layers: {result['layers']}")
        lines.append(f"  first loss: {losses[0]:.4f}")
        lines.append(f"  final loss: {losses[-1]:.4f}")
        lines.append(f"  output: dream_{preset_name}.png")
    lines.append("")
    lines.append("Explanation for presentation:")
    lines.append(
        "DeepDream does not train InceptionV3 weights. It freezes a pretrained CNN, "
        "then uses backpropagation to optimize the input image so selected hidden "
        "layers activate more strongly."
    )
    lines.append(
        "Different presets choose different convolution layers, so the same photo "
        "can produce gentle texture, classic neural patterns, or stronger dreamlike details."
    )
    (output_dir / "run_report.txt").write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    args = parse_args()
    if args.iterations < 1:
        raise ValueError("--iterations must be at least 1")
    if args.octaves < 1:
        raise ValueError("--octaves must be at least 1")
    if args.max_size < 128:
        raise ValueError("--max-size must be at least 128 for InceptionV3")

    start_time = perf_counter()
    image_path = resolve_path(args.image)
    output_dir = ensure_output_dir(args.output_dir)
    devices = prepare_runtime(args.seed)
    preset_names = get_requested_presets(args.preset)

    print("TensorFlow DeepDream Camera")
    print(f"TensorFlow version: {tf.__version__}")
    print(f"Keras cache: {os.environ.get('KERAS_HOME')}")
    print(f"Devices: {', '.join(devices) if devices else 'No devices reported'}")
    print(f"Input image: {image_path}")
    print(f"Output dir: {output_dir}")

    original_pixels, image_size = load_image_pixels(image_path, args.max_size)
    original_path = output_dir / "original.png"
    save_array_image(original_pixels.astype("uint8"), original_path)

    source_image = preprocess_pixels(original_pixels)
    feature_model = build_feature_model(preset_names)

    results: dict[str, dict[str, object]] = {}
    for preset_name in preset_names:
        result = run_deepdream(
            source_image=source_image,
            preset_name=preset_name,
            feature_model=feature_model,
            iterations=args.iterations,
            octaves=args.octaves,
            octave_scale=args.octave_scale,
            step=args.step,
        )
        results[preset_name] = result
        save_array_image(result["final"], output_dir / f"dream_{preset_name}.png")
        save_progress_grid(result, output_dir)

    save_comparison_grid(original_pixels.astype("uint8"), results, output_dir)
    elapsed_seconds = perf_counter() - start_time
    write_report(
        output_dir=output_dir,
        args=args,
        image_path=image_path,
        image_size=image_size,
        devices=devices,
        results=results,
        elapsed_seconds=elapsed_seconds,
    )

    print("\nDone.")
    print(f"Saved original image: {original_path}")
    for preset_name in results:
        print(f"Saved dream image: {output_dir / ('dream_' + preset_name + '.png')}")
    print(f"Saved comparison grid: {output_dir / 'comparison_grid.png'}")
    print(f"Saved report: {output_dir / 'run_report.txt'}")
    print(f"Elapsed: {elapsed_seconds:.2f}s")


if __name__ == "__main__":
    main()
