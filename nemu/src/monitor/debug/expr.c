#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
	NOTYPE = 256, 
	DNUM = 257,
	HNUM = 258,
	EQ = 259,
	NEQ = 260,
	AND = 261,
	OR = 262,
	REG = 263,
	DEREF = 264,
	MINUS = 265,

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */
	
	{" +",  NOTYPE},                // spaces
	{"0x[0-9a-fA-F]+", HNUM},	// hexadeciaml-number
	{"[0-9]+", DNUM},               // deciaml-numbers
	{"\\$[a-z]+", REG},				// reg_name
	{"==", EQ},						// equal
	{"!=", NEQ},					// not euual
	{"[&]{2}", AND},				// logical and
	{"[|]{2}", OR},					// logical or
	{"\\(", '('},					// left pth
	{"\\)", ')'},					// right pth
	{"!", '!'},						// not
	{"\\*", '*'},                   // multi     
	{"/", '/'},                   	// devi 
	{"\\+", '+'},					// plus
	{"-", '-'},						// subtraction

};
#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case 257:
					case 258:
					case 263:	tokens[nr_token].type=rules[i].token_type;
								strncpy(tokens[nr_token].str,substr_start,substr_len);
								nr_token++;
								break;
					case 259:
					case 260:
					case 261:
					case 262:
					case '(':
					case ')':
					case '!':
					case '*':
					case '/':
					case '+':	
					case '-':	tokens[nr_token].type=rules[i].token_type;
					 	  		nr_token++;
				   				break;
					case 256:	break;			
					default: panic("please implement me");
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

//check the expr generally
bool match_check(int p, int q) {
	//printf("%d %d\n",p,q);
	int ileft=0;
	bool flag=true;

	for(;p<=q;p++) {
		if(tokens[p].type=='('){
			ileft++;
			//printf("ileft++%d\n",ileft);
		}
		else if(tokens[p].type==')') {
			if(ileft>0) {
				ileft--;
				//printf("ileft--%d\n",ileft);
			}
			else {
				flag=false;
				//printf("flag=false\n");
				break;
			}
		}
	}
	if(ileft!=0)
		flag=false;
	return flag;
}

int check_parentheses(int p, int q) {
	//return value
	//-1:totally wrong expr(pths not match at all)
	//1:right expr with outer pths
	//0:right expr without outer pths(considered as false)
	
	if(match_check(p,q)==false) //->(1+2)))*(((3+4)
		return -1;
	else if(tokens[p].type!='(' || tokens[q].type!=')') //->1*(2+3)
		return 0;
	else if(!match_check(p+1,q-1)) //->(1+2)*(3+4)
		return 0;
	else return 1;

}

int domi_operator(int p, int q) {
	int temp=-1, flag=0, op=0;

	for(;p<=q;p++) {
		if(tokens[p].type=='(')		flag--;
		else if(tokens[p].type==')')	flag++;
		else if(tokens[p].type==262 && flag>=0 && op<=12) {
			temp=p;
			op=12;	}
		else if(tokens[p].type==261 && flag>=0 && op<=11) {
			temp=p;
			op=11;	}
		else if((tokens[p].type==259||tokens[p].type==260) && flag>=0 && op<=7) {
			temp=p;
			op=7;	}
		else if((tokens[p].type=='+'||tokens[p].type=='-') && flag>=0 && op<=4) {
			temp=p;
			op=4;	}
		else if((tokens[p].type=='*'||tokens[p].type=='/') && flag>=0 && op<=3) {
			temp=p;
			op=3;	}
		else if((tokens[p].type=='!'||tokens[p].type==264||tokens[p].type==265) && flag>=0 && op<=2) {
			temp=p;
			op=2;
		}	
	}

	return temp;
}

uint32_t str_to_hexi(char *str) {
	int i,len=strlen(str);
	uint32_t hexi=0;
	
	for(i=2;i<len;i++) {
		hexi *=16;
		if(str[i]>='0' && str[i]<='9')
			hexi +=str[i]-'0';
		else if(str[i]>='a' && str[i]<='f')
			hexi +=str[i]-'a'+10;
		else if(str[i]>='A' && str[i]<='F')
			hexi +=str[i]-'A'+10;
		else {
			printf("invalid number\n");
			return 0;
		}
	}
	return hexi;
}

uint32_t eval(int p, int q) {
	//printf("%d %d\n",p,q);
	if(p>q) {
		if(tokens[p].type=='!'||tokens[p].type==264||tokens[p].type==265)
			return 0;
		else
			assert(0);
	}
	else if(p==q) {
		uint32_t number;
		if(tokens[p].type==257) {
			number=(uint32_t)atoi(tokens[p].str);
			return number;
		}
		else if(tokens[p].type==258) { 
			sscanf(tokens[p].str,"%x",&number);
			return number;
		}
		else if(tokens[p].type==263) {
				if(strcmp(tokens[p].str,"$eax")==0)
					return cpu.gpr[0]._32;
				else if(strcmp(tokens[p].str,"$ecx")==0)
					return cpu.gpr[1]._32;
				else if(strcmp(tokens[p].str,"$edx")==0)
					return cpu.gpr[2]._32;
				else if(strcmp(tokens[p].str,"$ebx")==0)
					return cpu.gpr[3]._32;
				else if(strcmp(tokens[p].str,"$esp")==0)
					return cpu.gpr[4]._32;
				else if(strcmp(tokens[p].str,"$ebp")==0)
					return cpu.gpr[5]._32;
				else if(strcmp(tokens[p].str,"$esi")==0)
					return cpu.gpr[6]._32;
				else if(strcmp(tokens[p].str,"$edi")==0)
					return cpu.gpr[7]._32;
				else if(strcmp(tokens[p].str,"$eip")==0)
					return cpu.eip;
				else {
					printf("no such registers\n");
					return 0;
				}
		}
		else
			assert(0);

	}
	else if(check_parentheses(p,q)==1) {

		return eval(p+1,q-1);
	}
	else if(check_parentheses(p,q)==0) {
		int op=domi_operator(p,q);
		uint32_t val1=eval(p,op-1);
		uint32_t val2=eval(op+1,q);
		//uint32_t temp1,temp2;

		int op_type=tokens[op].type;
		switch(op_type) {
			case '+': return val1+val2;
			case '-': return val1-val2;
			case '*': return val1*val2;
			case '/': return val1/val2;
			case '!': return !val2;
									
			case 259: return val1==val2;
			case 260: return val1!=val2;
			case 261: return val1&&val2;
			case 262: return val1||val2;
			case 264: return swaddr_read(val2,4);
			default: assert(0);
		}
	}
	else {
		printf("bad expression2\n");
		return -1;
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	int i;
	int p=nr_token;
	for(i=0;i<p;i++) {
		if(tokens[i].type=='*' && (i==0||tokens[i-1].type==259||tokens[i-1].type==260||tokens[i-1].type==261||tokens[i-1].type==262||tokens[i-1].type=='!'||tokens[i-1].type=='*'||tokens[i-1].type=='/'||tokens[i-1].type=='+'||tokens[i-1].type=='-'))
			tokens[i].type=DEREF;
	}


	return eval(0,p-1);
}

