#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

Display *d;
Window window;

typedef struct {unsigned short r, g, b;}MyPixel;
typedef struct {int x, y;}MyPos;

MyPixel getPixel(MyPos pos);
MyPos getMousePos();
int checkPixels(XImage* image, int width, MyPixel pixel);

int main(int argc, char** argv)
{
    d = XOpenDisplay((char *) NULL);
    window = XRootWindow (d, XDefaultScreen (d));
    
    int keyboard_fd;
    keyboard_fd = open(argv[1], O_RDONLY);
    struct input_event ev;

    int state = 0;

    MyPos look_PosY, look_PosStart, look_PosEnd;
    MyPixel *look_pixels = new MyPixel[256];
    int look_count = 0;

    cout << "Where to look for pixel Y?" << endl;
    while(1)
    {
        read(keyboard_fd, &ev, sizeof(struct input_event));

        if(ev.type == 1)
        {
            //cout << "Key " << ev.code << " state " << ev.value << endl;
            if (ev.code == 16 && ev.value == 0){
                switch(state){
                    case 0:
                        look_PosY = getMousePos();
                        state = 1;
                        cout << "Looking for Pixels on Y:" << look_PosY.y << endl << "Where to start Search? (X-Coord.)" << endl;
                        break;
                    case 1:
                        look_PosStart = getMousePos();
                        state = 2;
                        cout << "Begin at X:" << look_PosStart.x << endl << "Where to end search? (X-Coord.)" << endl;
                        break;
                    case 2:
                        look_PosEnd = getMousePos();
                        state = 3;
                        cout << "End at X:" << look_PosEnd.x << endl << "Press Q on Colors to jump on. End with W." << endl;
                        break;
                    case 3:
                        look_pixels[look_count] = getPixel(getMousePos());
                        look_count++;
                        cout << "Added Color R:" << look_pixels[look_count-1].r << " G:" << look_pixels[look_count-1].r << " B:" << look_pixels[look_count-1].r << " to list." << endl;
                        break;
                }
            }
            if(ev.code == 17 && ev.value == 0)
            {
                cout << "Finished setup. Start game!" << endl;
                break;
            }
        }
    }

    clock_t start;
    start = clock();

    XImage *image;
    while(1)
    {
        image = XGetImage (d, window, look_PosStart.x, look_PosY.y, look_PosEnd.x-look_PosStart.x, 1, AllPlanes, XYPixmap);
        for(int i = 0; i < look_count; i++)
        {
            if(checkPixels(image, look_PosEnd.x-look_PosStart.x, look_pixels[i]) && (clock()-start)/(double)CLOCKS_PER_SEC > 0.33)
            {
                unsigned int keycode = XKeysymToKeycode(d, XK_space);
                XTestFakeKeyEvent(d, keycode, True, 0);
                XTestFakeKeyEvent(d, keycode, False, 0);
                XFlush(d);
                cout << "Jumped, waiting..." << endl;
                start = clock();
            }
        }
        XFree(image);
    }
    return 0;
}

int checkPixels(XImage* image, int width, MyPixel pixel)
{
    for(int i = 0; i < width; i++)
    {
        XColor c;
        c.pixel = XGetPixel(image, i, 0);
        XQueryColor (d, XDefaultColormap(d, XDefaultScreen (d)), &c);
        if(pixel.r == c.red && pixel.g == c.green && pixel.b == c.blue)
        {
            return 1;
        }
    }
    return 0;
}

MyPixel getPixel(MyPos pos)
{
    XImage *image;
    image = XGetImage (d, window, pos.x, pos.y, 1, 1, AllPlanes, XYPixmap);
    XColor c;
    c.pixel = XGetPixel (image, 0, 0);
    XFree (image);
    XQueryColor (d, XDefaultColormap(d, XDefaultScreen (d)), &c);
    return {c.red, c.green, c.blue};
}

MyPos getMousePos()
{
    XImage *image;
    Window window_returned;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int mask_return;
    XQueryPointer(d, window, &window_returned, &window_returned, &root_x, &root_y, &win_x, &win_y, &mask_return);
    return {root_x, root_y};
}
