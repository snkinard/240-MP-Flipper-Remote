# Contributing to 240-MP Remote

Focused fixes, documentation improvements, and reproducible bug reports are welcome.

For a larger feature, open an issue before investing substantial work. Do not include credentials, pairing secrets, Bluetooth addresses, private hostnames, or unrelated personal information in reports or logs.

## Make a change

Create a focused branch from `main`, follow the style of nearby code, and run the relevant checks:

```sh
make setup
make test
make lint
make build
```

These checks do not exercise Bluetooth or host integration. If you test on hardware, state the Flipper firmware, app revision, host environment, and behavior you observed. If you do not, identify the affected behavior that remains untested.

Keep copied or adapted code and assets traceable to their source and license.

## AI-assisted contributions

AI tools are allowed, but contributors must own and understand everything they submit. Project communication—including pull-request descriptions, code comments, review replies, and issue comments—must be authored and submitted by the human contributor; autonomous AI agents must not communicate with maintainers.

Pull requests must disclose which parts were AI-generated or AI-assisted and describe the human review and testing performed before submission. Pull requests that omit this disclosure may be closed without review.

### Creative Assets

Creative assets—including artwork, icons, illustrations, animations, audio, and similar media—must have documented provenance and licensing compatible with inclusion and redistribution under this project's GPL-3.0 license.

Human-created assets are preferred, whether created by the contributor or sourced from a third party under an appropriate license. Creative assets generated entirely by AI will generally not be accepted. Exceptions may be considered when unusual circumstances make an AI-generated asset uniquely appropriate and all provenance and licensing concerns have been adequately addressed.

## Pull requests

Explain the problem and change, link related issues, and report the testing performed. Keep unrelated changes out of the pull request.

Contributions are licensed under the repository's [GPL-3.0 license](LICENSE).
