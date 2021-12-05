__asm__(".symver xyz_old,xyz@VER_1");
__asm__(".symver xyz_new,xyz@@VER_2");

int xyz_old() { return 3; }
int xyz_new() { return 4; }

