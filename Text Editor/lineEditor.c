#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

typedef struct Buffer
{
    char *text;
    int lineNo;
    struct Buffer *next;
} Buffer;

typedef struct Action
{
    int linePos;
    int wordPos;
    int taskId;
    char *text;
    char *oldWord;
    char *newWord;
    struct Action *next;
} Action;

Buffer *head = NULL;
Buffer *tail = NULL;
Action *undoHead = NULL;
Action *redoHead = NULL;
int currentLine = 1;
int lineCount = 0;

Buffer *createBufferNode(int lineNo);
Buffer *getNodeAtPosition(int pos);
Action *createUndoRedoNode();
int displayMenu();
void displayFile();
void loadFile(FILE *fp);
void writeDataTofile(FILE *fp);
void insertLine(int pos, char *text, int afterCursor, int flag);
void insertWord(int wordPos, char *newWord, int flag);
void deleteLine(int flag);
void deleteWord(char *word, int flag);
void updateWord(char *oldWord, char *newWord, int flag);
void undoTask();
void redoTask();
void undoStack(int taskId, int linePos, int wordPos, char *text, char *oldWord, char *newWord);
void redoStack(int taskId, int linePos, int wordPos, char *text, char *oldWord, char *newWord);

Buffer *createBufferNode(int lineNo)
{
    Buffer *newNode = (Buffer *)malloc(sizeof(Buffer));
    if (newNode == NULL)
    {
        printf("Failed to create new node.\n");
        return NULL;
    }
    newNode->text = NULL;
    newNode->lineNo = lineNo;
    newNode->next = NULL;
    return newNode;
}

Action *createUndoRedoNode()
{
    Action *newNode = (Action *)malloc(sizeof(Action));
    if (newNode == NULL)
    {
        printf("Failed to create new node of Action\n");
        return NULL;
    }
    newNode->text = NULL;
    newNode->oldWord = NULL;
    newNode->newWord = NULL;
    newNode->next = NULL;
    return newNode;
}

void displayFile()
{
    printf("====>File Content<====\n\n");
    if (head == NULL || (head == tail && head->text[0] == '\n'))
    {
        printf("(Empty file)\n");
        return;
    }
    Buffer *temp = head;
    while (temp != NULL)
    {
        if (temp->lineNo == currentLine)
            printf("--> ");

        printf("%d) %s", temp->lineNo, temp->text);
        temp = temp->next;
    }
    printf("\n");
}

void loadFile(FILE *fp)
{
    char str[100];
    if (fp == NULL)
    {
        printf("File not opened\n");
        return;
    }
    while (fgets(str, sizeof(str), fp) != NULL)
    {
        Buffer *newNode = createBufferNode(++lineCount);
        newNode->text = (char *)malloc(sizeof(char) * (strlen(str) + 1));
        strcpy(newNode->text, str);

        if (head == NULL)
        {
            head = newNode;
            tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
    }
    displayFile();
}

Buffer *getNodeAtPosition(int pos)
{
    Buffer *current = head;
    while (current != NULL && current->lineNo != pos)
        current = current->next;

    return current;
}

void insertLine(int pos, char *text, int afterCursor, int flag)
{
    if (pos < 1 || pos > lineCount + 1)
    {
        printf("Invalid position. Line inserted at the end.\n");
        pos = lineCount + 1;
    }

    if (afterCursor)
        pos = currentLine + 1;
    else
        pos = currentLine;

    Buffer *newNode = createBufferNode(pos);
    newNode->text = (char *)malloc(sizeof(char) * (strlen(text) + 2));
    strcpy(newNode->text, text);

    if (text[strlen(text) - 1] != '\n')
        strcat(newNode->text, "\n");

    if (pos == 1 || currentLine == 0)
    {
        newNode->next = head;
        head = newNode;
    }
    else
    {
        Buffer *prev = getNodeAtPosition(pos - 1);
        newNode->next = prev->next;
        prev->next = newNode;
    }

    Buffer *temp = newNode->next;
    while (temp != NULL)
    {
        temp->lineNo++;
        temp = temp->next;
    }

    lineCount++;

    if (flag)
        undoStack(1, pos, 1, text, NULL, NULL);

    currentLine = pos;
}

void insertWord(int wordPos, char *newWord, int flag)
{
    if (currentLine < 1 || currentLine > lineCount)
    {
        printf("Current line does not exist.\n");
        return;
    }

    Buffer *lineBuffer = getNodeAtPosition(currentLine);
    if (!lineBuffer)
    {
        printf("Error: Unable to retrieve line at position %d.\n", currentLine);
        return;
    }

    char *lineText = lineBuffer->text;
    int len = strlen(lineText);
    int wordLen = strlen(newWord);

    int currentWord = 0;
    int charPos = 0;

    for (int i = 0; i < len; i++)
    {
        if (lineText[i] == ' ' || lineText[i] == '\n')
        {
            currentWord++;
            if (currentWord == wordPos)
            {
                charPos = i + 1;
                break;
            }
        }
    }

    if (wordPos == 1)
        charPos = 0;

    else if (wordPos > currentWord)
        charPos = len;

    char *newText = (char *)malloc(len + wordLen + 2);
    if (!newText)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    if (charPos == len)
    {
        strncpy(newText, lineText, charPos - 1);
        strcat(newText, " ");
    }
    else
        strncpy(newText, lineText, charPos);

    newText[charPos] = '\0';
    strcat(newText, newWord);

    if (charPos == len)
        strcat(newText, "\n");
    else
        strcat(newText, " ");

    strcat(newText, lineText + charPos);

    free(lineBuffer->text);
    lineBuffer->text = (char *)malloc(strlen(newText) + 1);
    if (!lineBuffer->text)
    {
        printf("Memory allocation failed.\n");
        free(newText);
        return;
    }
    strcpy(lineBuffer->text, newText);

    free(newText);

    if (flag)
        undoStack(4, currentLine, wordPos, newWord, NULL, NULL);
}

void deleteLine(int flag)
{
    if (currentLine < 1 || currentLine > lineCount)
    {
        printf("Line number does not exist.\n");
        return;
    }

    Buffer *current = head;
    Buffer *prev = NULL;

    if (currentLine == 1)
        head = head->next;

    else
    {
        for (int i = 1; i < currentLine; i++)
        {
            prev = current;
            current = current->next;
        }
        prev->next = current->next;
    }

    if (flag)
        undoStack(3, currentLine, 1, current->text, NULL, NULL);

    Buffer *temp = prev ? prev->next : head;
    while (temp != NULL)
    {
        temp->lineNo--;
        temp = temp->next;
    }

    lineCount--;

    currentLine = (currentLine > 1) ? currentLine - 1 : 1;
}

void deleteWord(char *word, int flag)
{
    if (currentLine < 1 || currentLine > lineCount)
    {
        printf("Line number does not exist.\n");
        return;
    }

    Buffer *lineBuffer = getNodeAtPosition(currentLine);
    char *lineText = lineBuffer->text;
    char *newText;
    int len = strlen(lineText);
    int wordLen = strlen(word);

    char *pos = strstr(lineText, word);
    if (pos == NULL)
    {
        printf("Word not found in the specified line.\n");
        return;
    }

    int wordPos = 1;
    for (char *p = lineText; p < pos; p++)
    {
        if (*p == ' ')
            wordPos++;
    }

    newText = (char *)malloc(len - wordLen + 1);
    strncpy(newText, lineText, pos - lineText);
    newText[pos - lineText] = '\0';
    strcat(newText, pos + wordLen);

    if (flag)
        undoStack(2, currentLine, wordPos, word, NULL, NULL);

    free(lineBuffer->text);
    lineBuffer->text = (char *)malloc(strlen(newText) + 1);
    strcpy(lineBuffer->text, newText);

    free(newText);
}

void updateWord(char *oldWord, char *newWord, int flag)
{
    Buffer *lineBuffer = getNodeAtPosition(currentLine);
    char *lineText = lineBuffer->text;
    char *newText;
    int len = strlen(lineText);
    int oldWordLen = strlen(oldWord);
    int newWordLen = strlen(newWord);

    char *pos = strstr(lineText, oldWord);
    if (pos == NULL)
    {
        printf("Word not found in the specified line.\n");
        return;
    }

    int wordPos = 1;
    for (char *p = lineText; p < pos; p++)
    {
        if (*p == ' ')
            wordPos++;
    }

    newText = (char *)malloc(len - oldWordLen + newWordLen + 1);
    strncpy(newText, lineText, pos - lineText);
    newText[pos - lineText] = '\0';
    strcat(newText, newWord);
    strcat(newText, pos + oldWordLen);

    if (flag)
        undoStack(5, currentLine, wordPos, NULL, oldWord, newWord);

    free(lineBuffer->text);
    lineBuffer->text = (char *)malloc(strlen(newText) + 1);
    strcpy(lineBuffer->text, newText);

    free(newText);
}

void undoTask()
{
    if (!undoHead)
    {
        printf("No actions to undo.\n");
        return;
    }

    Action *action = undoHead;
    undoHead = undoHead->next;

    switch (action->taskId)
    {
    case 1:
        currentLine = action->linePos;
        deleteLine(0);
        break;
    case 2:
        currentLine = action->linePos;
        insertWord(action->wordPos, action->text, 0);
        break;
    case 3:
        insertLine(action->linePos, action->text, 0, 0);
        break;
    case 4:
        currentLine = action->linePos;
        deleteWord(action->text, 0);
        break;
    case 5:
        currentLine = action->linePos;
        updateWord(action->newWord, action->oldWord, 0);
        break;
    default:
        printf("Invalid task id\n");
        break;
    }

    redoStack(action->taskId, action->linePos, action->wordPos, action->text, action->oldWord, action->newWord);

    free(action->text);
    free(action->oldWord);
    free(action->newWord);
    free(action);
}

void redoTask()
{
    if (!redoHead)
    {
        printf("No actions to redo.\n");
        return;
    }

    Action *action = redoHead;
    redoHead = redoHead->next;

    switch (action->taskId)
    {
    case 1:
        insertLine(action->linePos, action->text, 0, 0);
        break;
    case 2:
        currentLine = action->linePos;
        deleteWord(action->text, 0);
        break;
    case 3:
        currentLine = action->linePos;
        deleteLine(0);
        break;
    case 4:
        currentLine = action->linePos;
        insertWord(action->wordPos, action->text, 0);
        break;
    case 5:
        currentLine = action->linePos;
        updateWord(action->oldWord, action->newWord, 0);
        break;
    default:
        printf("Invalid task id\n");
        break;
    }

    undoStack(action->taskId, action->linePos, action->wordPos, action->text, action->oldWord, action->newWord);

    free(action->text);
    free(action->oldWord);
    free(action->newWord);
    free(action);
}

void undoStack(int taskId, int linePos, int wordPos, char *text, char *oldWord, char *newWord)
{
    Action *newNode = createUndoRedoNode();
    newNode->taskId = taskId;
    newNode->linePos = linePos;
    newNode->wordPos = wordPos;
    if (text != NULL)
        newNode->text = strdup(text);

    if (oldWord != NULL)
        newNode->oldWord = strdup(oldWord);

    if (newWord != NULL)
        newNode->newWord = strdup(newWord);

    newNode->next = undoHead;
    undoHead = newNode;

    while (redoHead != NULL)
    {
        Action *temp = redoHead;
        redoHead = redoHead->next;
        free(temp->text);
        free(temp->oldWord);
        free(temp->newWord);
        free(temp);
    }
}

void redoStack(int taskId, int linePos, int wordPos, char *text, char *oldWord, char *newWord)
{
    Action *newNode = createUndoRedoNode();
    newNode->taskId = taskId;
    newNode->linePos = linePos;
    newNode->wordPos = wordPos;
    if (text != NULL)
        newNode->text = strdup(text);

    if (oldWord != NULL)
        newNode->oldWord = strdup(oldWord);

    if (newWord != NULL)
        newNode->newWord = strdup(newWord);

    newNode->next = redoHead;
    redoHead = newNode;
}

int displayMenu()
{
    displayFile();

    printf("\nWhich Task You Want to Perform on this file\n\n");
    printf("1. Insert Text Line\n");
    printf("2. Insert Word\n");
    printf("3. Delete Text Line\n");
    printf("4. Delete Word\n");
    printf("5. Update Text Line\n");
    printf("6. Move Cursor\n");
    printf("7. Undo Task\n");
    printf("8. Redo Task\n");
    printf("9. Exit\n\n");

    printf("Enter Your Choice: ");
    int choice;
    int pos;
    int wordPos;
    int linePos;
    char text[100];
    char oldWord[50], newWord[50];
    char word[50];
    scanf("%d", &choice);
    switch (choice)
    {
    case 1:
        if (lineCount == 0)
        {
            printf("Enter text line: ");
            scanf(" %[^\n]s", text);
            insertLine(1, text, 0, 1);
        }
        else
        {
            printf("Insert Text Line Before (0) or After (1) Cursor: ");
            int option;
            scanf("%d", &option);
            printf("Enter text line: ");
            scanf(" %[^\n]s", text);
            insertLine(currentLine, text, option, 1);
        }
        break;
    case 2:
        printf("Enter Word Position : ");
        scanf("%d", &pos);
        printf("Enter new word: ");
        scanf("%s", newWord);
        insertWord(pos, newWord, 1);
        break;
    case 3:
        deleteLine(1);
        break;
    case 4:
        printf("Enter Word : ");
        scanf(" %[^\n]s", newWord);
        deleteWord(newWord, 1);
        break;
    case 5:
        printf("Enter Old Word : ");
        scanf(" %[^\n]s", oldWord);
        printf("Enter New Word : ");
        scanf(" %[^\n]s", newWord);
        updateWord(oldWord, newWord, 1);
        break;
    case 6:
        printf("Enter line number to move cursor: ");
        scanf("%d", &linePos);
        if (linePos < 1 || linePos > lineCount)
        {
            printf("Invalid line number.\n");
            break;
        }
        currentLine = linePos;
        break;
    case 7:
        undoTask();
        break;
    case 8:
        redoTask();
        break;
    case 9:
        return 0;
        break;
    default:
        printf("Invalid choice\n");
        break;
    }
    getch();

    return 1;
}

void writeDataTofile(FILE *fp)
{
    Buffer *temp = head;
    while (temp != NULL)
    {
        fprintf(fp, "%s", temp->text);
        temp = temp->next;
    }
}

int main(int argc, char *argv[])
{
    if (argc > 3)
    {
        printf("Error: More than 3 arguments not allowed\n");
        for (int i = 0; i < argc; i++)
            printf("%s\n", argv[i]);
        exit(0);
    }

    FILE *fp;
    char filePath[100];
    int choice;

    if (argc == 1)
    {
        strcpy(filePath, "file.txt");
        fp = fopen(filePath, "w+");
        if (fp == NULL)
        {
            printf("Error: file not open\n");
            exit(0);
        }
    }
    else if (argc == 2)
    {
        strcpy(filePath, argv[1]);

        fp = fopen(filePath, "r+");
        if (fp == NULL)
        {
            printf("Error: file does not exist\n");
            printf("Creating file with the same name\n");
            fp = fopen(filePath, "w+");
            if (fp == NULL)
            {
                printf("Error: could not create file\n");
                exit(0);
            }
        }
        printf("File opened\n");
        loadFile(fp);
    }
    else
    {
        strcpy(filePath, argv[2]);
        strcat(filePath, "\\");
        strcat(filePath, argv[1]);

        fp = fopen(filePath, "r+");

        if (fp == NULL)
        {
            printf("Error: file does not exist\n");
            printf("Creating file with the same name\n");
            fp = fopen(filePath, "w+");
            if (fp == NULL)
            {
                printf("Error: could not create file\n");
                exit(0);
            }
        }
        printf("File opened\n");
        loadFile(fp);
    }
    fclose(fp);

    system("cls");

    while (displayMenu())
        system("cls");

    printf("Do you want to Save the Changes => (0) NO (1) Yes : ");
    scanf("%d", &choice);

    if (choice)
    {
        fp = fopen(filePath, "w+");
        writeDataTofile(fp);
    }

    return 0;
}