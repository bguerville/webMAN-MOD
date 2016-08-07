static void enable_classic_ps2_mode(void);
static void disable_classic_ps2_mode(void);

static void enable_classic_ps2_mode(void)
{
	savefile(PS2_CLASSIC_TOGGLER, NULL, 0);
}

static void disable_classic_ps2_mode(void)
{
	cellFsUnlink((char*)PS2_CLASSIC_TOGGLER);
}
