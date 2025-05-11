# Waybar Media Player CFFI module

# Usage

## Building this module

```bash
meson setup build
meson compile -C build
```

## Load the module

Edit your waybar config:
```json
{
	// ...
	"modules-center": [
		// ...
		"cffi/mediaplayer"
	],
	// ...
	"cffi/mediaplayer": {
		// Path to the compiled dynamic library file
		"module_path": "/home/<user>/.config/waybar/scripts/waybar_mediaplayer.so"
	}
}
```

## Customizing

Edit your style.css
```css
#mediaplayer {
}

#mediaplayer .button {
}

#mediaplayer .play-button{
}

#mediaplayer .prev-button {
}

#mediaplayer .next-button {
}

#mediaplayer .players {
}

#mediaplayer .title {
}
```
