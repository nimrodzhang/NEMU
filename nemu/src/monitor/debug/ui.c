#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

int str2num(char *str) {
	int l=strlen(str);
	int i,j;
	for(i=0;i<l;i++)
		if(str[i]<48||str[i]>57)
			return -1;

	int num=0;
	for(j=0;j<l;j++) {
		num=num+str[j]-48;
		num=num*10;
	}
	num=num/10;
	return num;
}

static int cmd_si(char *args) {
	if(args==NULL) {
		cpu_exec(1);
		return 0;
	}
	else {
		int tem=str2num(args);
		if(tem==-1) {
			//printf("-1!!!");
			return 0;
		}
		else {
			//printf("%d\n",tem);
			cpu_exec(tem);
			return 0;
		}
	}		
}

static int cmd_info(char *args) {
	if(strcmp(args,"r")==0) {
		printf("eax\t0x%x\t%d\n", cpu.gpr[0]._32, cpu.gpr[0]._32);
		printf("ecx\t0x%x\t%d\n", cpu.gpr[1]._32, cpu.gpr[1]._32);
		printf("edx\t0x%x\t%d\n", cpu.gpr[2]._32, cpu.gpr[2]._32);
		printf("ebx\t0x%x\t%d\n", cpu.gpr[3]._32, cpu.gpr[3]._32);
		printf("esp\t0x%x\t0x%x\n", cpu.gpr[4]._32, cpu.gpr[4]._32);
		printf("ebp\t0x%x\t0x%x\n", cpu.gpr[5]._32, cpu.gpr[5]._32);
		printf("esi\t0x%x\t%d\n", cpu.gpr[6]._32, cpu.gpr[6]._32);
		printf("edi\t0x%x\t%d\n", cpu.gpr[7]._32, cpu.gpr[7]._32);
		printf("eip\t0x%x\t0x%x\n", cpu.eip, cpu.eip);
	}
	else if(strcmp(args,"w")==0)
		show_wp();
	return 0;
}	

static int cmd_x(char *args) {
	char *bef, *beh;
	int steps,i;
	uint32_t memo;
	bool flag;

	bef=strtok(args," ");
	beh=strtok(NULL," ");
	
	steps=atoi(bef);
	memo=expr(beh,&flag);
	if(flag==false)
		printf("invalid input");
	//printf("%d\n",steps);
	//printf("0x%x\n",memo);
	else {
		for(i=0;i<steps;i++) {
			uint32_t n_memo=memo+4*i;
			printf("0x%x\t",n_memo);
			printf("0x%08x\n",swaddr_read(n_memo,4));
		}
	}
	return 0;
}

static int cmd_p(char *args) {
	bool flag=true;
	uint32_t value;
	value = expr(args,&flag);

	if(flag==false) {
		printf("failed");
	}
	else {
		printf("0x%x\n", value);
	}

	return 0;
}

static int cmd_w(char *args) {
	bool success=true;
	WP *cre=new_wp();
	strcpy(cre->name,args);
	cre->old_value=expr(args,&success);
	cre->new_value=cre->old_value;
	if(success==false) {
		printf("bad expression, watch falied\n");
		clear_wp(cre);
		free_wp(cre);
	}
	return 0;
}

static int cmd_d(char *args) {
	int n=atoi(args);
	bool flag=find_to_del(n);
	if(flag==true) {
		printf("watchpoint removed\n");
		return 0;
	}
	else {
		printf("remove failed\n");
		return 0;
	}
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Single step", cmd_si },
	{ "info", "Print registers or watchpoint", cmd_info },
	{ "x", "Scanf memory", cmd_x },
	{ "p", "Calculate the value of a expression", cmd_p },
	{ "w", "Set watchpoints", cmd_w },
	{ "d", "Delete watchpoints", cmd_d },
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
