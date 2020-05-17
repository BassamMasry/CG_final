## How to build
g++ -o app Body_w_camera.cpp -lGL -lGLU -lglut -lm
./app

## How to use
### Camera movements
Use p to toggle between zoom/move and pan/pinch.

In zoom mode:
- Up arrow: zoom in
- Down arrow: zoom out
- Left arrow: move forward
- Right arrow: move backwards

In not zoom mode (pan mode):
- Up arrow: pan right
- Down arrow: pan left
- Left arrow: pinch up
- Right arrow: pinch down

### Body movements
s and shift+s for shoulder movement

e and shift+e for elbow movement

f and shift+f for lower finger movement

g and shift+g for upper finger movement

h and shift+h for right hip movement

j and shift+j for left hip movement

a and shift+a for right hip abduction/adduction

q and shift+q for right hip abduction/adduction

k and shift+k for right knee movement

l and shift+l for left knee movement
