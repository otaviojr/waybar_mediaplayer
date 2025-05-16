# Waybar Media Player CFFI module

<p align="center">
  <img src="https://github.com/otaviojr/waybar_mediaplayer/blob/master/docs/example.png" alt="Example">
</p>

* Support for multiple players
* Previous and Next buttons when supported
* Progress bar
* will show only when a player becomes available
* Compact: Titles will scroll

# Usage

## Building this module

```bash
meson setup build
meson compile -C build
cp build/waybar_mediaplayer.so /home/<user>/.config/waybar/scripts
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
		"scroll-title": true,
		"scroll-interval": 200,
		"scroll-before-timeout":5,
		"title-max-width": 200,
		"scroll-step": 2
		"tooltip": true,
		"tooltip-image-width": 300,
		"tooltip-image-height": 300,
		"btn-play-icon": "",
		"btn-pause-icon": "",
		"btn-prev-icon": "",
		"btn-next-icon": ""
	}
}
```

## Customizing

Edit your style.css
```css
#mediaplayer {
    color: red;
}

#mediaplayer .button {
    color: white;
    background: transparent;
    border-width: 0px;
    border-radius: 0px;
    margin: 0px;
    padding: 0px;
    box-shadow: none;
    text-shadow: none;
}

#mediaplayer .play-button{
}

#mediaplayer .prev-button {
}

#mediaplayer .next-button {
}

#mediaplayer .players {
     color: white;
}

#mediaplayer .title {
     color: white;
}
```
