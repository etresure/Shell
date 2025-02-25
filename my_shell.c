
//исходя из примеров в постановке задачи, я предполагаю, что вложенность скобок недопустима, т.е.
//невозможно создать сабшелл внутри существующего сабшелла
//также в данной реализации предполагается, что фоновый режим может находиться только в конце команды
//перенаправление ввода/вывода строго в начале и в конце команды соответственно
//перенаправление ввода используется только с командой cat: cat <f.txt

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <ctype.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_PINK "\x1b[95m"

typedef struct start_struct 
{
    char word[50][255];
    char command[50][3];
} parce;

int flag;
int i_main; // Индекс word
int sub_start;
int sub_end;
parce structura; // Структура
int check;
void build(char *);
void execution();
int f_and_or(int, int);
int f_file(int, int);
int f_pipe(int, int);
int exec(int, int, int);
void handle_background(pid_t);
void print_structure();
int subshell;
int background;
int sub_back;
int if_and;
int conv;
void full_print()
{
    char *user = getenv("USER");         //getenv - значение переменной среды
    char host[256];
    gethostname(host, _SC_HOST_NAME_MAX);                    //имя машины
    printf(COLOR_PINK "%s@%s> " COLOR_RESET, user, host);
    fflush(stdout);
}

void print_structure() 
{
    printf("Structure:\n");
    for (int i = 0; i <= i_main; i++) 
    {
        printf("Word[%d]: '%s'\n", i, structura.word[i]);
        if (structura.command[i][0] != '\0') 
            printf("Command[%d]: '%s'\n", i, structura.command[i]);
    }
    printf("\n");
}

int main() 
{
    char start_str[255];

    while (1) 
    {
        for (int j = 0; j < 50; j++) 
        {
            memset(structura.word[j], 0, sizeof(char) * 255); // Устанавливаем значения
            memset(structura.command[j], 0, sizeof(char) * 3);
        }

        full_print();
        fflush(stdout);
        if (!fgets(start_str, sizeof(start_str), stdin)) 
			exit(0);

        int len_start = strlen(start_str);
        if (start_str[len_start - 1] == '\n') 
			start_str[len_start - 1] = 0;
        if (!strcmp(start_str, "exit")) 
			exit(0);
		background = 0;
		subshell = 0;
		sub_back = 0;
		if_and = 0;
		conv = 0;
        build(start_str);
        print_structure();
        fflush(stdout);
        execution();
    }
}
void trim(char *str) 
{
    char *end;
    int len;

    char *start = str;  // Удаление ведущих пробелов
    while (isspace((unsigned char)*start)) 
		start++;

    if (start != str)  // Если строка начинается с пробелов, сдвигаем её "влево"
    {
        len = strlen(start);
        for (int i = 0; i <= len; i++) 
            str[i] = start[i];
    }

    end = str + strlen(str) - 1;    // Удаление завершающих пробелов
    while (end >= str && isspace((unsigned char)*end)) 
		end--;
    *(end + 1) = '\0';
}

void build(char *start_str) {
    i_main = 0;
    int i_cmd = 0; // Индекс внутри команды
    int i = 0;     // Индекс в строке
	check = 0;
	int in_text = 0;
    while (start_str[i] != 0) 
    {
        if ((start_str[i] == '&') && (start_str[i + 1] == '&')) 
        {
            structura.command[i_main][0] = '&';
            structura.command[i_main][1] = '&';
            i += 2;
            i_main++;
            i_cmd = 0;
            if_and = 1;
        } 
        else if ((start_str[i] == '|') && (start_str[i + 1] == '|')) 
        {
            structura.command[i_main][0] = '|';
            structura.command[i_main][1] = '|';
            i += 2;
            i_main++;
            i_cmd = 0;
            if_and = 1;
        }   
        else if ((start_str[i] == 'c') && (start_str[i + 1] == 'd')) 
        {
            structura.word[i_main][0] = 'c';
            structura.word[i_main][1] = 'd';
            i += 2;
            i_main++;
            i_cmd = 0;
        } 
        else if ((start_str[i] == '>') && (start_str[i + 1] == '>')) 
        {
            structura.command[i_main][0] = '>';
            structura.command[i_main][1] = '>';
            i += 2;
            i_main++;
            i_cmd = 0;
        } 
        else if (start_str[i] == '(') 
        {
			subshell = 1;
			structura.command[i_main][0] = '(';
			i_main++;
			i++;
			i_cmd = 0;
		} 
		else if (start_str[i] == ')') 
		{
			structura.command[i_main][0] = ')';
			i_main++;
			i++;
			i_cmd = 0;
		}
		else if (start_str[i] == '&' && !in_text) 
		{
			background = 1;
			structura.command[i_main][0] = start_str[i];
            i_main++;
            i_cmd = 0;
            i++;
            if (!strcmp(structura.command[i_main - 2], ")"))
				sub_back = 1;
		}
		else if (start_str[i] == '|')
		{
			structura.command[i_main][0] = start_str[i];
            i_main++;
            i_cmd = 0;
            i++;
            conv = 1;
		}
        else 
        {
			if (start_str[i] == '"')
				in_text = 1;
            switch (start_str[i]) 
            {
                case '(':
                case ')':
                //case '|':
                case '>':
                case '<':
                case ';':
                    structura.command[i_main][0] = start_str[i];
                    i_main++;
                    i_cmd = 0;
                    i++;
                    break;
                default:
                    structura.word[i_main][i_cmd++] = start_str[i++];
                    break;
            }
        }
    }
	if (start_str[strlen(start_str)-1] == '&')
		check = 1;
    if (structura.command[i_main][0] == '\0') 
        structura.command[i_main][0] = ';';
    
    for (int j = 0; j <= i_main; j++) // Удаление пробелов
    {
        trim(structura.word[j]);
        trim(structura.command[j]);
    }
}

void handle_cd(char *path) 
{
    char *home = getenv("HOME");

    if (!strcmp(path, "~")) // Если `path` содержит только `~`, переходим в домашний каталог
    {
        if (chdir(home) != 0)
            perror("cd");
        return;
    }

    trim(path); // Убираем пробелы из начала и конца пути
    if (chdir(path) != 0) // Если переход в указанный путь не удался, выводим ошибку
        perror("cd");  
}

void executionS(int start, int end) 
{
	int i_execution;
    int saved_stdout = dup(STDOUT_FILENO); // Сохраняем стандартный вывод
    int saved_stdin = dup(STDIN_FILENO);  // Сохраняем стандартный ввод
    if (saved_stdout < 0 || saved_stdin < 0) 
    {
        perror("Error saving original descriptors");
        return;
    }
	
    for (i_execution = start; i_execution <= end; i_execution++) 
    {
        if (!strcmp(structura.word[start], "cd")) 
        {
            if (start + 1 <= i_main)
                handle_cd(structura.word[start + 1]);
            else 
                handle_cd("~"); // Если путь не указан, переход в домашний каталог

            start = i_execution + 2; // Переходим к следующей команде
            i_execution++;
            continue;
        } 
        else if (!strcmp(structura.command[i_execution], ";")) 
        {
            end = i_execution + 1;
            f_and_or(start, end);
            start = end;

            if (dup2(saved_stdout, STDOUT_FILENO) < 0) // Восстанавливаем stdout и stdin
                perror("Error restoring stdout in execution");
            if (dup2(saved_stdin, STDIN_FILENO) < 0) 
                perror("Error restoring stdin in execution");
            
        }
    }

    if (start <= end) 
    {
        f_and_or(start, end);
        if (dup2(saved_stdout, STDOUT_FILENO) < 0) // Восстанавливаем stdout и stdin
            perror("Error restoring stdout at end");
        if (dup2(saved_stdin, STDIN_FILENO) < 0) 
            perror("Error restoring stdin at end");
    }

    close(saved_stdout);
    close(saved_stdin);
}


void execution() 
{
    int end;
	int i_execution = 0;
	int start = i_execution;
    int saved_stdout = dup(STDOUT_FILENO); // Сохраняем стандартный вывод
    int saved_stdin = dup(STDIN_FILENO);  // Сохраняем стандартный ввод
	int result = 0;
    if (saved_stdout < 0 || saved_stdin < 0) 
    {
        perror("Error saving original descriptors");
        return;
    }

    for (i_execution = 0; i_execution <= i_main; i_execution++) 
    {
        if (!strcmp(structura.word[start], "cd")) // Проверяем, является ли команда "cd"
        {
            if (start + 1 <= i_main)
                handle_cd(structura.word[start + 1]);
            else 
                handle_cd("~"); // Если путь не указан, переход в домашний каталог
            start = i_execution + 2; // Переходим к следующей команде
            i_execution++;
            continue;
        }
        else if (!strcmp(structura.command[i_execution], "&")) 
        {
			background = 0;
			if (!result)
			{ 
				end = i_execution;
				pid_t pid_son = fork();
				if (pid_son == 0) 
				{
					pid_t pid_grandson = fork();  // В процессе сына создаем внука
					if (pid_grandson == 0) // Внук
					{
						signal(SIGINT, SIG_IGN);   // Игнорируем SIGINT
						setpgid(0, 0);             // Создаем новую группу процессов
						close(STDIN_FILENO);       // Отключаем ввод
						printf("Background process started with PID %d.\n", getpid());
						fflush(stdout);	
						if (f_and_or(start, end)) 
						{
							printf("\nBackground process finished successfully with PID %d.\n", getpid());
							full_print();
							fflush(stdout);
							exit(0);
						} 
						else 
						{
							printf("\nBackground process finished with unsupported error with PID %d.\n", getpid());
							fflush(stdout);
							exit(1);
						}
					} 
					else if (pid_grandson > 0) 
					{
						//printf("\nson died\n");
						//fflush(stdout);
						exit(0); // Сын завершает выполнение
					} 
					else 
					{
						perror("fork failed (grandson)");
						exit(1);	
					}
				} 
				else if (pid_son > 0) // Родительский процесс
				{
					waitpid(pid_son, NULL, 0); // Ожидаем завершения сына
					start = end;
					if (check)
					{
						start = i_execution + 2;
						i_execution++;
					}
				} 
				else 
					perror("fork failed (son)");
			}
			else
				start = i_execution + 1;
        } 
        else if (!strcmp(structura.command[i_execution], "(")) 
        {
			if (!result)
			{
				subshell = 0;
				//printf("Start subshell\n");
				fflush(stdout);
				int sub_start = i_execution + 1;
				int sub_end = sub_start; // Найти соответствующую закрывающую скобку
				int brackets = 1;
				while (sub_end <= i_main && brackets > 0) 
				{
					if (!strcmp(structura.command[sub_end], "(")) brackets++;
					if (!strcmp(structura.command[sub_end], ")")) brackets--;
					sub_end++;
				}
				sub_end--; // Закрывающая скобка
				if (brackets != 0) 
				{
					fprintf(stderr, "Unmatched parentheses\n");
					return;
				}
				if (!sub_back)
				{
					printf("Start subshell\n");
					fflush(stdout);
					pid_t pid = fork();  // Создаем новый процесс для сабшелла
					if (pid == 0) // Сабшелл
					{
						executionS(sub_start, sub_end);
						exit(0); // Завершаем сабшелл
					} 
					else if (pid > 0) 
					{
						int status; // Ожидание завершения сабшелла
						waitpid(pid, &status, 0);
						if (WEXITSTATUS(status) != 0)
							fprintf(stderr, "Subshell failed\n");
						result = !WEXITSTATUS(status);
						printf("Done\n");
						fflush(stdout);
						i_execution = sub_end; // Переход к следующей команде
						start = i_execution + 1;
						//i_execution++;
					} 
					else
						perror("fork failed");
				}
				else
				{
					pid_t pid_son = fork();
					if (pid_son == 0) 
					{
						pid_t pid_grandson = fork();  // В процессе сына создаем внука
						if (pid_grandson == 0) // Внук
						{
							signal(SIGINT, SIG_IGN);   // Игнорируем SIGINT
							setpgid(0, 0);             // Создаем новую группу процессов
							close(STDIN_FILENO);       // Отключаем ввод
							printf("Background process started with PID %d.\n", getpid());
							fflush(stdout);
							executionS(sub_start, sub_end);
							printf("\nBackground process finished successfully with PID %d.\n", getpid());
							full_print();
							fflush(stdout);
							exit(0);
						} 
						else if (pid_grandson > 0) 
						{
							//printf("\nson died\n");
							//fflush(stdout);
							exit(0); // Сын завершает выполнение
						} 
						else 
						{
							perror("fork failed (grandson)");
							exit(1);	
						}
					} 
					else if (pid_son > 0) // Родительский процесс
					{
						waitpid(pid_son, NULL, 0); // Ожидаем завершения сына
						return;
					} 
					else 
						perror("fork failed (son)");
				}
			}
			else
			{
				int sub_start = i_execution + 1;
				int sub_end = sub_start; // Найти соответствующую закрывающую скобку
				int brackets = 1;
				while (sub_end <= i_main && brackets > 0) 
				{
					if (!strcmp(structura.command[sub_end], "(")) brackets++;
					if (!strcmp(structura.command[sub_end], ")")) brackets--;
					sub_end++;
				}
				sub_end--; // Закрывающая скобка
				if (brackets != 0) 
				{
					fprintf(stderr, "Unmatched parentheses\n");
					return;
				}
				i_execution = sub_end;
				start = i_execution + 1;
				result = 1;
			
			}
		} 
        else if (!strcmp(structura.command[i_execution], ";")) 
        {
            end = i_execution;
            f_and_or(start, end);
            start = end + 1;

            if (dup2(saved_stdout, STDOUT_FILENO) < 0) // Восстанавливаем stdout и stdin
                perror("Error restoring stdout in execution");
            if (dup2(saved_stdin, STDIN_FILENO) < 0) 
                perror("Error restoring stdin in execution");
        }
        else if (subshell || background)
        {
			if (!result)
			{
				end = i_execution;
				result = f_and_or(start, end);
				fflush(stdout);
				start = end + 1;
				if (dup2(saved_stdout, STDOUT_FILENO) < 0) // Восстанавливаем stdout и stdin
					perror("Error restoring stdout at end");
				if (dup2(saved_stdin, STDIN_FILENO) < 0) 
					perror("Error restoring stdin at end");
			}
		}
	}
    if (start <= i_main) 
    {	
		if (!result)
        {
			f_and_or(start, i_main);
			
			if (dup2(saved_stdout, STDOUT_FILENO) < 0) // Восстанавливаем stdout и stdin
				perror("Error restoring stdout at end");
			if (dup2(saved_stdin, STDIN_FILENO) < 0) 
				perror("Error restoring stdin at end");
        }
        
    }
    close(saved_stdout);
    close(saved_stdin);
}

int f_and_or(int start, int end) 
{
    int result = 0;  // Результат выполнения команды (0 - успех, !=0 - ошибка)
    for (int i = start; i <= end; i++) 
    {
        if (!strcmp(structura.command[i], "&&"))  // Выполняем команду перед &&
        {
            result = f_file(start, i - 1);
            if (result != 0)
                return 0; // Если команда завершилась с ошибкой, возвращаем 0
            start = i + 1;   // Переходим к следующей команде
        } 
        else if (!strcmp(structura.command[i], "||")) 
        {
            result = f_file(start, i - 1);// Выполняем команду перед ||
            if (result == 0) 
                return 1; // Если команда завершилась успешно, возвращаем 1
            start = i + 1;  // Переходим к следующей команде
        }
    }
    if (start <= end)   // Выполняем последнюю команду
        return f_file(start, end);

    return result;
}

int f_file(int start, int end) 
{
    int input_redirected = 0;
    int output_redirected = 0;
    int input_fd = -1, output_fd = -1;
	int result = 0;
    int saved_stdout = dup(STDOUT_FILENO);  // Сохраняем стандартные дескрипторы
    int saved_stdin = dup(STDIN_FILENO);
    if (saved_stdout < 0 || saved_stdin < 0) 
    {
        perror("Error saving original descriptors");
        return 0;
    }

    for (int i = start; i <= end; i++) // Проходим по командам и проверяем на перенаправление
    {
        if (!strcmp(structura.command[i], "<")) 
        {
            input_fd = open(structura.word[i + 1], O_RDONLY); // Перенаправление ввода
            if (input_fd < 0) 
            {
                perror("Error opening file for read");
                goto cleanup;
            }
            if (dup2(input_fd, STDIN_FILENO) < 0) 
            {
                perror("Error redirecting stdin");
                close(input_fd);
                goto cleanup;
            }
            close(input_fd);
            input_redirected = 1;

        } 
        else if (!strcmp(structura.command[i], ">") || !strcmp(structura.command[i], ">>")) 
        {
            // Перенаправление вывода
            output_fd = open(structura.word[i + 1], O_WRONLY | O_CREAT | (!strcmp(structura.command[i], ">>") ? O_APPEND : O_TRUNC), 0666);
            if (output_fd < 0) 
            {
                perror("Error opening file for write");
                goto cleanup;
            }
            if (dup2(output_fd, STDOUT_FILENO) < 0) 
            {
                perror("Error redirecting stdout");
                close(output_fd);
                goto cleanup;
            }
            close(output_fd);
            output_redirected = 1;
        }
    }

    result = f_pipe(start, end);  // Выполняем пайп
cleanup:     // Восстанавливаем дескрипторы
    if (input_redirected) dup2(saved_stdin, STDIN_FILENO);
    if (output_redirected) dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    close(saved_stdin);

    return result;
}


int f_pipe(int start, int end) 
{
    int fd[2];
    int prev_fd = -1; // Для передачи данных между командами
    pid_t pid;
	int flag = 1;
    for (int i = start; i <= end; i++) 
    {
        if (i == end || !strcmp(structura.command[i], "|")) 
        {
			flag = 0;
            if (i != end && pipe(fd) < 0) 
            {
                perror("Error creating pipe");
                return 0;
            }

            pid = fork();
            if (pid == 0) // Дочерний процесс
            { 
                if (prev_fd != -1) // Если есть предыдущий пайп
                { 
                    dup2(prev_fd, STDIN_FILENO);
                    close(prev_fd);
                }
                if (i != end) // Если это не последняя команда
                { 
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                }
                exec(start, prev_fd, fd[1]); // Выполняем команду
                perror("exec failed");
                exit(1);
            } 
            else if (pid > 0) // Родительский процесс
            { 
                if (prev_fd != -1) close(prev_fd); // Закрываем предыдущий пайп
                if (i != end) close(fd[1]);       // Закрываем запись в текущем пайпе
                prev_fd = fd[0];                  // Сохраняем чтение для следующей команды
                start = i + 1;                    // Переходим к следующей команде
            } 
            else 
            {
                perror("fork failed");
                return 0;
            }
        }
    }
   if (flag || (conv && if_and))
    {
		pid = fork();
		if (pid == 0) // Дочерний процесс
		{
			if (start == end && structura.word[start][0] == '\0') 
			{
				fprintf(stderr, "f_file: empty command\n");
				exit(1); // Ошибка: пустая команда
			}
			exec(start, STDIN_FILENO, STDOUT_FILENO); // Выполняем команду
			perror("exec failed");
			exit(1); // Выходим, если exec завершился ошибкой
		} 
		else if (pid > 0) 
		{
			// Родительский процесс
			int status;
			waitpid(pid, &status, 0); // Ожидание завершения дочернего процесса
			if (WIFEXITED(status)) 
				return WEXITSTATUS(status); // Возвращаем код завершения
			else
				return -1; // Процесс завершился некорректно
		} 
		else 
		{
			perror("fork failed"); // Ошибка при создании процесса
			return -1;
		}
	}
    while ((pid = waitpid(-1, NULL, 0)) > 0);  // Ждем завершения всех дочерних процессов
    
    return 1;
}

void correct_strtok(char *command_line, char **argv, int *argc) 
{
    char *ptr = command_line;
    char *arg_start;
    int in_quotes = 0;
    *argc = 0;

    while (*ptr) 
    {
        while (isspace((unsigned char)*ptr)) ptr++;

        if (*ptr == '\0') break;

        arg_start = ptr;
        in_quotes = 0;

        while (*ptr && (in_quotes || !isspace((unsigned char)*ptr))) 
        {
            if (*ptr == '"') 
            {
                in_quotes = !in_quotes;
                memmove(ptr, ptr+1, strlen(ptr));
            }
            else 
                ptr++;
        }

        if (*ptr) 
        {
            *ptr = '\0';
            ptr++;
        }

        argv[*argc] = arg_start;
        (*argc)++;
    }

    argv[*argc] = NULL;	
}

int exec(int i, int from, int to) 
{
    if (from != STDIN_FILENO) 
    {
        dup2(from, STDIN_FILENO);
        close(from);
    }
    if (to != STDOUT_FILENO) 
    {
        dup2(to, STDOUT_FILENO);
        close(to);
    }
    if (structura.word[i][0] == '\0') 
    {
        exit(1);
    }

    char *arr[256];
    int wcount = 0;
    correct_strtok(structura.word[i], arr, &wcount);
    execvp(arr[0], arr);

    perror("exec failed");
    exit(1);
}
