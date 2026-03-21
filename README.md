# Waybar Media Player CFFI module

<p align="center">
  <img src="https://otaviojr.github.io/waybar_mediaplayer/docs/example.png" alt="Example">
</p>

* Support for multiple players
* Previous and Next buttons when supported
* Progress bar
* will show only when a player becomes available
* Compact: Titles will scroll

# Usage

## Cloning repository

```bash
git clone https://github.com/otaviojr/waybar_mediaplayer.git waybar_mediaplayer
cd waybar_mediaplayer
```

## Building this module

```bash
meson setup build
meson compile -C build
cp build/waybar_mediaplayer.so /home/<user>/.config/waybar/scripts
```

## Dependencies

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
		"btn-next-icon": "",
		"ignored-players": "playerctl"
	}
}
```

## Customizing

Edit your style.css
```css
#media_player {
    color: red;
}

#media_player .button {
    color: white;
    background: transparent;
    border-width: 0px;
    border-radius: 0px;
    margin: 0px;
    padding: 0px;
    box-shadow: none;
    text-shadow: none;
}

#media_player .play-button{
}

#media_player .prev-button {
}

#media_player .next-button {
}

#media_player .players {
     color: white;
}

#media_player .title {
     color: white;
}
```
