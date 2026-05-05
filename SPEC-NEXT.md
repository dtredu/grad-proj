
# Specification

## Hot to read this doc
This document specifies the next requirements for the next iteration of software to be implemented.

### Important terms:
- "virtual time" and "time" terms are used interchangably and are used as virtual abstraction over actual time:
  - it starts with value of `60` seconds when program starts
  - it increases with the same rate of program running
  - when simulation is paused, it doesn't increase

- Virtual frame (aka "camera frame") is essentially the name for state variables that are changed at `camera_frame_rate`:
  - its role is to snapshot the virtual time for the rendering

- Rendered frame is a frame to be rendered
  - It doesn't snapshot virtual time, what's to be rendered depends
    - Virtual frame (i.e. state variables)
    - Simulation parameters

## Simulation parameters (SimulationParameters struct)
by default things are **configurable** by user, if not **derived** from **configurable** parameters

### Display

- Emission type: `display_emission_type` (enum)
  - Progressive Emission (PE) - PWM follows raster scan, the `display_pwm_time`
  - Simultaneous Emission (SE) - pixels flicker simultaneously

- Dimensions(configurable):
   - Hactive = display width: `display_h_active`
   - Hblank = Hfront + Hsync + Hback: `display_h_blank`
   - Vactive = display height: `display_v_active`
   - Vblank = Vfront + Vsync + Vback: `display_v_blank`

- Timings (configurable):
  - Display time offset relative to camera time (i.e. current simulation time): `display_time_offset`
  - Frame frequency: `display_frame_freq`

- Timings (derived):
  - Frame time:
    `display_frame_time = 1 / display_frame_freq` 

  - Scanline frequency:
    `display_scanline_freq = 1 / display_scanline_time = display_frame_freq * (display_v_active + display_v_blank)`

  - Scanline time:
    `display_scanline_time = 1 / display_scanline_freq = display_frame_time / (display_v_active + display_v_blank)`

  - Pixel frequency:
    `display_pixel_freq = 1 / display_pixel_time = display_scanline_freq * (display_h_active + display_h_blank)`

  - Pixel time:
    `display_pixel_time = 1 / display_pixel_freq = display_scanline_time / (display_h_active + display_h_blank)`


- PWM:
  - PWM start offset: `display_pwm_start_offset`
  - PWM Duty percent: `display_pwm_duty` (between 0.0 and 1.0)
    - should specify how long light is emitted: `display_pwm_frame_time * display_pwm_duty` out of `display_pwm_frame_time` for every frame
  - PWM frequency: `display_pwm_freq`
  - PWM time (derived): `display_pwm_time`
    - `display_pwm_time = 1 / display_pwm_freq` 

- Extra metrics:
  - Bands (lines) Per Frame: `display_bands_per_frame`
    - For progressive emission: `display_bands_per_frame = display_frame_time / display_pwm_time` 
    - For simultaneous emission: `display_bands_per_frame = 1`

- Virtual scene specifics:
  - Display center should be located at 0 0 0
  - Texture width & height should be auto-derived so that following is true:
    - `display_texture_width:display_texture_height = display_h_active:display_v_active`
    - `display_texture_width * display_texture_height = 1.0`



### Camera

- Dimensions:
  - Camera matches viewport `camera_matches_viewport` (boolean)
    - True
    - False
  - Width: `camera_width`
    - Locked to viewport width if `camera_matches_viewport == true`
  - Height: `camera_height`
    - Locked to viewport height if `camera_matches_viewport == true`

- Shutter type: `camera_shutter_type` (enum)
  - Rolling
  - Global

- Timings (configurable):
  - Frames Per Second: `camera_frame_rate`
  - Frame exposure duty: `camera_frame_exposure_duty`
  - Scanline exposure duty: `camera_scanline_exposure_duty`

- Timings (derived):
  - Frame time: 
    `camera_frame_time = camera_frame_time = 1 / camera_frame_rate` 

  - Frame exposure time:
    `camera_frame_exposure_time = camera_frame_time * camera_frame_exposure_duty`

  - (rolling shutter only) Scanline time:
    `camera_scanline_time = canera_frame_exposure_time / camera_height`

  - (rolling shutter only) Scanline exposure time: 
    `camera_scanline_exposure_time = camera_frame_exposure_time * camera_scanline_exposure_duty`

- Virtual scene specifics:
  - Distance from display center: `camera_distance`
    - Default value: `5.0`
    - Controlled with:
      - Z/C on keyboard
  - Position angles:
    - Pitch: `camera_pos_pitch`
      - Default value: `0 degrees`
      - Controlled with:
        - W/S on keyboard
        - Dragging mouse cursor vertically while holding LMB
    - Yaw: `camera_pos_yaw`
      - Default value: `0 degrees`
      - Controlled with:
        - A/D on keyboard
        - Dragging mouse cursor horizontally while holding LMB
    - Roll: `camera_pos_roll`
      - Default value: `0 degrees`
      - Controlled with:
        - Q/E on keyboard
        - Dragging mouse cursor horizontally while holding RMB
  - Position & looking angle
    - Coordinates should be derived using the `camera distance` and position angles
    - Looking angles should be auto-derived from c so that camera faces towards display`



## Simulation


### State variables (SimulationState struct):
- Current camera frame start time: `camera_frame_start_time`
- Last display PWM start time `display_pwm_start_time`
  - `display_pwm_start_time <= camera_frame_start_time`  
  - already incorporates `display_pwm_start_offset`
  - when `display_pwm_start_offset` is changed, the delta is applied to `display_pwm_start_time`


### Time simulation

- Time windows change with the camera FPS.
- The program renders camera at max FPS, but:
  - Simulation state variables are updated each virtual camera frame, not each rendered frame
  - Simulation parameters can be updated each rendered frame, and upon updating everything is re-derived (or re-derived every rendered frame)

### Viewport and camera width
- By default camera width should follow viewport (i.e. `camera_matches_viewport = true`)
- But when `camera_matches_viewport = false`, the frame should be rendered in a camera resolution and scaled to the viewport resolution



### Misc information
- There should be the `P` keyboard key mapped to pause/resume the simulation
- There should be the `M` keyboard key mapped to hiding/showing egui
- By default the simulation should be paused
- The simulation starts when program starts, and `display_pwm_start_offset` means essentially the raster scan shift offset


## How? (extra thoughts)
### How camera's rendered frame is generated
To generate a complete camera frame you need to generate all scanlines, using their time windows.

### How camera's scanline should be generated
Generating camera's scanline of pixels would involve integrating functions of all these pixels in a time window of scanline.

### Display 
In virtual time each pixel on display is essentially a function that accepts all display parameters, current time, and display start offset, and returns the brightness value.



## Artifact Patterns

| Display Emission Type | Camera Shutter type | Visual Effect |
|---------|--------|---------------|
| Simultaneous | Global | Whole display flicker |
| Progressive | Global | Horizontal bands parallel to display |
| Simultaneous | Rolling | Horizontal bands parallel to camera sensor |
| Progressive | Rolling | Combined bands that may not be parallel to anything |
