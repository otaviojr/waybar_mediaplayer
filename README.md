# Waybar Media Player CFFI module

<p align="center">
  <img src="https://github.com/otaviojr/waybar_mediaplayer/blob/master/docs/example.png" alt="Example">
</p>

* Support for multiple players
* Previous and Next buttons when supported
* Progress bar
* will show only when a player becomes available

# Usage

## Building this module

```bash
meson setup build
meson compile -C build
```

## Dependencies

* playerctl
* Latest waybar version ( Arch: use the git version )

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
