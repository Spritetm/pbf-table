//Interface for backboard interaction

#define BBIMG_PF_LOGO 0
#define BBIMG_LOADING 1
#define BBIMG_TABLE(x) (2+(x))

//Show the given image (BBIMG_*) on the backbox.
void backboard_show(int img);
