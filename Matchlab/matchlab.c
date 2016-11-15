#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
/**
CS 4400 Matchlab
Author: Qixiang Chao
*/

int is_a_model(char* sequence)
{
	int index;
	int l_num = 0;
	int i;
	for(i = 0; sequence[i] != '\0'; i++)
	{
		if(sequence[i] == 'l')
		{
			index = i;
			l_num++;
			continue;
		}
		else if(sequence[i] != 'l' && (l_num >= 0 && l_num <= 3))//0 and 3 digits inclusive
		{
			index = i;
			break;
		}
		else return 0;
	}
	if(sequence[index + 1] == '\0') return 0;//Return 0 if incomplete
	int t_num = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(sequence[i] == 't')
		{
			t_num++;
			index = i;
			continue;
		}
		else if(sequence[i] != 't' && (t_num >= 1 && t_num <= 5))//1 and 5 digits inclusive
		{
			index = i;
			break;
		}
		else return 0;
	}
	int dig_num = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(isdigit(sequence[i]))
		{
			dig_num++;
			if(sequence[i + 1] == '\0' && (dig_num >= 1 && dig_num <= 3)) return 1;//1 and 3 digits inclusive
			continue;
		}
		else return 0;
	}
	return 0;
}

char* a_model_conversion(char sequence[])
{
	char* for_return;
	//Relpace each "D" to "F" and vice versa
	int i;
	for(i = 0; i < strlen(sequence); i++)
	{
		if(sequence[i] == 'D')
		{
			sequence[i] = 'F';
		}
		else if(sequence[i] == 'F')
		{
			sequence[i] = 'D';
		}
	}
	for_return = sequence;
	return for_return;
}

/**Helper method for reversing string**/
void str_reverse(char* str)
{
	int i = 0;
	size_t j = strlen(str) - 1;
	while(i < j)
	{
		char temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++;
		j--;
	}
}

int is_b_model(char* sequence)
{
	int i_number = 0;
	int upper_number = 0;
	int u_number = 0;
	int index = 0;
	int digit_count = 0;
	int i;
	for(i = 0; sequence[i] != '\0'; i++)
	{
		if(sequence[i] == 'i')
		{
			index = i;
			i_number++;
			continue;
		}
		else if(sequence[i] != 'i' && i_number >= 2)//2 or more repetitions of "i"
		{
			index = i;
			 break;
		}
		else return 0;
	}
	if(sequence[index + 1] == '\0') return 0;
	char* tem_str = (char*)malloc(10); //Set char pointer space up to 10 with 1 terminator
	int tem_index = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(isdigit(sequence[i]))
		{
			index = i;
			digit_count++;
			tem_str[tem_index++] = sequence[i];//Store digits to temporary char pointer 
			continue;
		}
		else if(!isdigit(sequence[i]) && (digit_count <= 3 && digit_count >= 1))//1 and 3 decimal digits 
		{
			index = i;
			break;
		}
		else return 0;
	}
	if(sequence[index + 1] == '\0') return 0;
	//digit_count = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(sequence[i] == 'u')
		{
			index = i;
			u_number++;
			continue;
		}
		else if(sequence[i] != 'u' && ((u_number & 1) == 1))
		{
			index = i;
			break;
		}
		else return 0;
	}
	if(sequence[index + 1] == '\0') return 0;
	char* temp_for_cmp = (char*)malloc(10);//Same size with previous temp char pointer
	str_reverse(tem_str);//Reverse previous string, keep for future comparison
	int tmp_index = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(isdigit(sequence[i]))
		{
			index = i;
			temp_for_cmp[tmp_index++] = sequence[i];
			continue;
		}
		else if(!isdigit(sequence[i]) && (strlen(temp_for_cmp) == strlen(tem_str)))//Two strings must have same size, false otherwise
		{
			index = i;
			break;
		}
		else return 0;
	}
	if(strcmp(temp_for_cmp, tem_str) != 0) return 0;//Return 0 if they are not the same string
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(isupper(sequence[i]))
		{
			upper_number++;
			if(sequence[i + 1] == '\0' && ((upper_number & 1) == 1)) return 1;//Check if ended right
		}
		else return 0;
	}
	return 0;
}

char* b_model_conversion(char* sequence)
{
	size_t space = strlen(sequence) + strlen(sequence);//The length of conversed string is two times bigger than origin
	char* for_return = (char*)malloc(space + 1);//For terminator
	for_return[0] = '\0';//Initializing, just in case for strcat()
	char* temp = (char*)malloc(2);//Temp char pointer only contains two bytes
	int i;
	for(i = 0; sequence[i] != '\0'; i++)
	{
		temp[0] = sequence[i];
		temp[1] = (i & 7) + '0';//i & 7 is same with i % 8
		strcat(for_return, temp);//Append to string
		free(temp);//Free memory
		temp = (char*)malloc(2);//Malloc again
	}
	for_return[space] = '\0';//Add termintor
	free(temp);
	return for_return;
}
/**Helper method, dealing with the even positioned**/
char* deal_positioned(char* positioned)
{
	int space = 1 + (strlen(positioned) >> 1);//String lenth divided by 2 and add 1 for space. "ABCDE" has "ACE"
	int tmp_ind = 0;
	char* for_return = (char*)malloc(space + 1);
	size_t i;
	for(i = 0; i < strlen(positioned); i+=2)//Add every other element from origin to temp
	{
		for_return[tmp_ind++] = positioned[i];
	}
	for_return[space] = '\0';//Termintor
	return for_return;
}

int is_c_model(char* sequence)
{
	int index = 0;
	int f_number = 0;
	int i;
	for(i = 0; sequence[i] != '\0'; i++)
	{
		if(sequence[i] == 'f')
		{
			index = i;
			f_number++;
			continue;
		}
		else if(sequence[i] != 'f' && ((f_number & 1) == 1))//f_number & 1 equals 1 if f_number is odd
		{
			index = i;
			break;
		}
		else return 0;
	}
	if(sequence[index + 1] == '\0') return 0;
	int upper_number = 0;
	char* positioned = (char*)malloc(255);
	int index_for_positioned = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(isupper(sequence[i]))
		{
			index = i;
			positioned[index_for_positioned++] = sequence[i];
			upper_number++;
			continue;
		}
		else if(!isupper(sequence[i]) && ((upper_number & 1) == 1))
		{
			index = i;
			break;
		}
		else return 0;
	}
	if(sequence[index + 1] == '\0') return 0;
	char* new_positioned = deal_positioned(positioned);
	int rep = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(sequence[i] == 'p')
		{
			index = i;
			rep++;
			continue;
		}
		else if((sequence[i] != 'p') && (rep >= 2))
		{
			index = i;
			break;
		}
		else return 0;
	}
	if(sequence[index + 1] == '\0') return 0;
	char* to_cmp = (char*)malloc(strlen(new_positioned) + 1);
	int this_temp_ind = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(isupper(sequence[i]) && (strlen(to_cmp) <= strlen(new_positioned)))
		{
			to_cmp[this_temp_ind++] = sequence[i];
			index = i;
			continue;
		}
		else if(!isupper(sequence[i]) && (strcmp(to_cmp, new_positioned) == 0))
		{
			index = i;
			break;
		}
		else return 0;
	}
	int dig_num = 0;
	for(i = index; sequence[i] != '\0'; i++)
	{
		if(isdigit(sequence[i]))
		{
			dig_num++;
			if((sequence[i + 1] == '\0') && (dig_num >= 1 && dig_num <= 3)) return 1;
			continue;
		}
		else return 0;
	}
	return 0;
}

/**Helper method for removing element at index, only take char array and int as parameters**/
void remove_char_at_index(char str[], int index)
{
	memmove(&str[index], &str[index + 1], strlen(str) - 1);
}

char* c_model_conversion(char sequence[])//Char array parameter, not yet figured out better way
{
	char* for_return;
	size_t first_index = 0;
	size_t last_index = 0;
	size_t i;
	for(i = 0; i < strlen(sequence); i++)
	{
		if(sequence[i] == 'A')
		{
			first_index = i;
			break;
		}
	}
	if(first_index == 0)
	{
		for_return = sequence;
		return for_return;
	}
	for(i = strlen(sequence) - 1; i >= 0; i--)
	{
		 if(sequence[i] == 'A')
		{
			last_index = i;
			break;
		}
	}
	if(last_index > 0)
	{
		remove_char_at_index(sequence, first_index);
		remove_char_at_index(sequence, last_index - 1);
	}
	else remove_char_at_index(sequence, first_index);
	for_return = sequence;
	return for_return;
}

int main(int argc, char** argv)
{
	if(argc <= 1) return 0;
	int i;
	if(argc >= 2 && strcmp(argv[1], "-a") != 0 && strcmp(argv[1], "-b") != 0 && strcmp(argv[1], "-c") != 0 && strcmp(argv[1], "-t") != 0)
	{
		for(i = 1; i <= argc - 1; i++)
		{
			if(is_a_model(argv[i]) == 1)
			{
				printf("Yes\n");
				continue;
			}
			else printf("No\n");
		}
	}
	else if(argc >= 3 && strcmp(argv[1], "-t") == 0 && strcmp(argv[2], "-a") != 0 && strcmp(argv[2], "-c") != 0 && strcmp(argv[2], "-b") != 0)
	{
		for(i = 2; i <= argc - 1; i++)
		{
			if(is_a_model(argv[i]) == 1)
			{
				printf("%s\n", a_model_conversion(argv[i]));
			}
		}
	}
	else if(argc >= 3 && strcmp(argv[1], "-a") == 0 && (strcmp(argv[2], "-t") != 0))
	{
		for(i = 2; i <= argc - 1; i++)
		{
			if(is_a_model(argv[i]) == 1)
			{
				printf("Yes\n");
				continue;
			}
			else printf("No\n");
		}
	}
	else if(argc >= 3 && strcmp(argv[1], "-b") == 0 && (strcmp(argv[2], "-t") != 0))
	{
		for(i = 2; i <= argc - 1; i++)
		{
			if(is_b_model(argv[i]) == 1)
			{
				printf("Yes\n");
				continue;
			}
			else printf("No\n");
		}
	}
	else if(argc >= 3 && strcmp(argv[1], "-c") == 0 && (strcmp(argv[2], "-t") != 0))
	{
		for(i = 2; i <= argc - 1; i++)
		{
			if(is_c_model(argv[i]) == 1)
			{
				printf("Yes\n");
				continue;
			}
			else printf("No\n");
		}
	}
	else if(argc >= 4 && ((strcmp(argv[1], "-a") == 0) && (strcmp(argv[2], "-t") == 0)) || ((strcmp(argv[2], "-a") == 0) && (strcmp(argv[1], "-t") == 0)))
	{
		for(i = 3; i <= argc - 1; i++)
		{
			if(is_a_model(argv[i]) == 1)
			{
				printf("%s\n", a_model_conversion(argv[i]));
			}
		}
	}
	else if(argc >= 4 && ((strcmp(argv[1], "-b") == 0) && (strcmp(argv[2], "-t") == 0)) || ((strcmp(argv[2], "-b") == 0) && (strcmp(argv[1], "-t") == 0)))
	{
		for(i = 3; i <= argc - 1; i++)
		{
			if(is_b_model(argv[i]) == 1)
			{
				printf("%s\n", b_model_conversion(argv[i]));
			}
		}
	}
	else if(argc >= 4 && ((strcmp(argv[1], "-c") == 0) && (strcmp(argv[2], "-t") == 0)) || ((strcmp(argv[2], "-c") == 0) && (strcmp(argv[1], "-t") == 0)))
	{
		for(i = 3; i <= argc - 1; i++)
		{
			if(is_c_model(argv[i]) == 1)
			{
				printf("%s\n", c_model_conversion(argv[i]));
			}
		}
	}
	return 0;
}
