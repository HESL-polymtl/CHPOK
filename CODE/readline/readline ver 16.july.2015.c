

#define BUFLEN 1024
static char buf[BUFLEN];

char *
readline(const char *prompt)
{
	int i, c, echoing;

	if (prompt != NULL)
		printf("%s", prompt);

	i = 0;
	while (1) {
		c = getchar();
		if (c < 0) {
			printf("read error\n");
			return NULL;
		} else if ((c == '\b' || c == '\x7f') && i > 0) {//if c=Backspace
			i--;	//Backspace dont work correctly,because it don't clear 
				//information on the screen
		} else if (c >= ' ' && i < BUFLEN-1) {//if c=simbol
			printf("%c",c);
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {//if c=Enter
			printf("\n");
			buf[i] = 0;
			return buf;
		}
	}
}

