# Digital-Clock-with-Arduino-Nano

A digital clock with ds3231 that display using max7219 and can set the clock with rotary encoder ky 040

## How To Use
- Hold 1.5s -> switch to setclock display/default display
- rotate -> change the number
- click -> change the mode (hour, minute, save)

## Flowchart
```mermaid
flowchart TD
    A([Start]) --> B[/Load time from DS3231/]
    B --> C[Clock display]
    C --> D{Rotary held\n> 1.5s?}
    D -- No --> C
    D -- Yes --> E[Clock set display\nhours]
    E --> F{Rotary\nclicked?}
    F -- No --> E
    F -- Yes --> G[Clock set display\nminutes]
    G --> H{Rotary\nclicked?}
    H -- No --> G
    H -- Yes --> I[Clock set display\nyes / no]
    I --> J{Rotary held\n> 1.5s?}
    J -- No --> E
    J -- Yes --> K{Confirm\nsave = yes?}
    K -- No --> C
    K -- Yes --> L[/Set time to DS3231\n+ buzzer/]
    L --> C
```
