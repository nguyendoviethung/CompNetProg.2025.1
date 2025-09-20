#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 100
#define MAX_LINE 256

typedef struct Student {
    char id[20];
    char lastname[50];
    char firstname[50];
    float p1, p2;   // điểm thành phần
    char grade[3];  // điểm chữ
    struct Student *next;
} Student;

typedef struct {
    char subjectID[20];
    char subjectName[100];
    int f1, f2; // hệ số %
    char semester[20];
    int studentCount;
    Student *head;
} ScoreBoard;

/* ===================== LINKED LIST UTILS ===================== */
Student *createStudent(char *id, char *lastname, char *firstname, float p1, float p2, char *grade) {
    Student *s = (Student*)malloc(sizeof(Student));
    strcpy(s->id, id);
    strcpy(s->lastname, lastname);
    strcpy(s->firstname, firstname);
    s->p1 = p1;
    s->p2 = p2;
    strcpy(s->grade, grade);
    s->next = NULL;
    return s;
}

void addStudent(ScoreBoard *sb, Student *s) {
    s->next = sb->head;
    sb->head = s;
    sb->studentCount++;
}

/* ===================== I/O FILE ===================== */
void writeScoreBoardToFile(ScoreBoard *sb) {
    char filename[100];
    sprintf(filename, "%s_%s.txt", sb->subjectID, sb->semester);
    FILE *f = fopen(filename, "w");
    if (!f) { printf("Cannot write file\n"); return; }

    fprintf(f, "SubjectID|%s\n", sb->subjectID);
    fprintf(f, "Subject|%s\n", sb->subjectName);
    fprintf(f, "F|%d|%d\n", sb->f1, sb->f2);
    fprintf(f, "Semester|%s\n", sb->semester);
    fprintf(f, "StudentCount|%d\n", sb->studentCount);

    Student *cur = sb->head;
    while(cur) {
        fprintf(f, "S|%s|%s|%s|%.1f|%.1f|%s|\n",
                cur->id, cur->lastname, cur->firstname, cur->p1, cur->p2, cur->grade);
        cur = cur->next;
    }
    fclose(f);
}

ScoreBoard* readScoreBoardFromFile(char *subjectID, char *semester) {
    char filename[100];
    sprintf(filename, "%s_%s.txt", subjectID, semester);
    FILE *f = fopen(filename, "r");
    if (!f) { printf("File not found!\n"); return NULL; }

    ScoreBoard *sb = (ScoreBoard*)malloc(sizeof(ScoreBoard));
    sb->head = NULL;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0; // remove newline
        if (strncmp(line, "SubjectID", 9) == 0) {
            sscanf(line, "SubjectID|%s", sb->subjectID);
        } else if (strncmp(line, "Subject", 7) == 0) {
            sscanf(line, "Subject|%[^\n]", sb->subjectName);
        } else if (line[0] == 'F') {
            sscanf(line, "F|%d|%d", &sb->f1, &sb->f2);
        } else if (strncmp(line, "Semester", 8) == 0) {
            sscanf(line, "Semester|%s", sb->semester);
        } else if (strncmp(line, "StudentCount", 12) == 0) {
            sscanf(line, "StudentCount|%d", &sb->studentCount);
        } else if (line[0] == 'S') {
            char id[20], lastname[50], firstname[50], grade[3];
            float p1, p2;
            char *tok = strtok(line, "|"); // "S"
            tok = strtok(NULL, "|"); strcpy(id, tok);
            tok = strtok(NULL, "|"); strcpy(lastname, tok);
            tok = strtok(NULL, "|"); strcpy(firstname, tok);
            tok = strtok(NULL, "|"); p1 = atof(tok);
            tok = strtok(NULL, "|"); p2 = atof(tok);
            tok = strtok(NULL, "|"); strcpy(grade, tok);

            Student *s = createStudent(id, lastname, firstname, p1, p2, grade);
            addStudent(sb, s);
        }
    }
    fclose(f);
    return sb;
}

/* ===================== FUNCTIONS ===================== */
void addNewScoreBoard() {
    ScoreBoard sb;
    sb.head = NULL;
    sb.studentCount = 0;

    printf("Enter subject ID: "); scanf("%s", sb.subjectID);
    getchar();
    printf("Enter subject name: "); fgets(sb.subjectName, sizeof(sb.subjectName), stdin);
    sb.subjectName[strcspn(sb.subjectName, "\n")] = 0;
    printf("Enter weight F1 F2 (%%): "); scanf("%d %d", &sb.f1, &sb.f2);
    printf("Enter semester: "); scanf("%s", sb.semester);

    writeScoreBoardToFile(&sb);
    printf("Score board created!\n");
}

void addScore() {
    char subjectID[20], semester[20];
    printf("Enter subject ID: "); scanf("%s", subjectID);
    printf("Enter semester: "); scanf("%s", semester);

    ScoreBoard *sb = readScoreBoardFromFile(subjectID, semester);
    if (!sb) return;

    char id[20], lastname[50], firstname[50], grade[3];
    float p1, p2;
    printf("Enter student ID: "); scanf("%s", id);
    printf("Enter lastname: "); scanf("%s", lastname);
    printf("Enter firstname: "); scanf("%s", firstname);
    printf("Enter score1, score2: "); scanf("%f %f", &p1, &p2);
    printf("Enter grade (A-F): "); scanf("%s", grade);

    Student *s = createStudent(id, lastname, firstname, p1, p2, grade);
    addStudent(sb, s);
    writeScoreBoardToFile(sb);
    printf("Student added!\n");
}

void removeScore() {
    char subjectID[20], semester[20], sid[20];
    printf("Enter subject ID: "); scanf("%s", subjectID);
    printf("Enter semester: "); scanf("%s", semester);
    printf("Enter student ID to remove: "); scanf("%s", sid);

    ScoreBoard *sb = readScoreBoardFromFile(subjectID, semester);
    if (!sb) return;

    Student *cur = sb->head, *prev = NULL;
    while (cur) {
        if (strcmp(cur->id, sid) == 0) {
            if (prev) prev->next = cur->next;
            else sb->head = cur->next;
            free(cur);
            sb->studentCount--;
            printf("Removed student %s\n", sid);
            break;
        }
        prev = cur; cur = cur->next;
    }
    writeScoreBoardToFile(sb);
}

void searchScore() {
    char subjectID[20], semester[20], sid[20];
    printf("Enter subject ID: "); scanf("%s", subjectID);
    printf("Enter semester: "); scanf("%s", semester);
    printf("Enter student ID to search: "); scanf("%s", sid);

    ScoreBoard *sb = readScoreBoardFromFile(subjectID, semester);
    if (!sb) return;

    Student *cur = sb->head;
    while(cur) {
        if (strcmp(cur->id, sid) == 0) {
            printf("Found: %s %s %s %.1f %.1f %s\n",
                   cur->id, cur->lastname, cur->firstname, cur->p1, cur->p2, cur->grade);
            return;
        }
        cur = cur->next;
    }
    printf("Not found!\n");
}

void displayScoreBoardAndReport() {
    char subjectID[20], semester[20];
    printf("Enter subject ID: "); scanf("%s", subjectID);
    printf("Enter semester: "); scanf("%s", semester);

    ScoreBoard *sb = readScoreBoardFromFile(subjectID, semester);
    if (!sb) return;

    printf("Score board for %s - %s:\n", sb->subjectID, sb->semester);
    Student *cur = sb->head;
    int countA=0,countB=0,countC=0,countD=0,countF=0;
    float total=0, maxScore=-1, minScore=11;
    char topName[100]="", lowName[100]="";

    while(cur) {
        float avg = (cur->p1*sb->f1 + cur->p2*sb->f2)/100;
        printf("%s %s %s %.1f %.1f %s (%.2f)\n",
               cur->id, cur->lastname, cur->firstname, cur->p1, cur->p2, cur->grade, avg);
        total += avg;
        if (avg > maxScore) { maxScore = avg; sprintf(topName, "%s %s", cur->lastname, cur->firstname); }
        if (avg < minScore) { minScore = avg; sprintf(lowName, "%s %s", cur->lastname, cur->firstname); }

        if (strcmp(cur->grade,"A")==0) countA++;
        else if (strcmp(cur->grade,"B")==0) countB++;
        else if (strcmp(cur->grade,"C")==0) countC++;
        else if (strcmp(cur->grade,"D")==0) countD++;
        else countF++;
        cur = cur->next;
    }

    printf("\nReport:\n");
    printf("The student with the highest mark is: %s\n", topName);
    printf("The student with the lowest mark is: %s\n", lowName);
    printf("The average mark is: %.2f\n", total/sb->studentCount);
    printf("\nHistogram:\n");
    printf("A: "); for(int i=0;i<countA;i++) printf("*"); printf("\n");
    printf("B: "); for(int i=0;i<countB;i++) printf("*"); printf("\n");
    printf("C: "); for(int i=0;i<countC;i++) printf("*"); printf("\n");
    printf("D: "); for(int i=0;i<countD;i++) printf("*"); printf("\n");
    printf("F: "); for(int i=0;i<countF;i++) printf("*"); printf("\n");
}

/* ===================== MAIN ===================== */
int main() {
    int choice;
    while(1) {
        printf("\nLearning Management System\n");
        printf("1. Add a new score board\n");
        printf("2. Add score\n");
        printf("3. Remove score\n");
        printf("4. Search score\n");
        printf("5. Display score board and score report\n");
        printf("Other to quit\n");
        printf("Your choice: ");
        scanf("%d", &choice);
        switch(choice) {
            case 1: addNewScoreBoard(); break;
            case 2: addScore(); break;
            case 3: removeScore(); break;
            case 4: searchScore(); break;
            case 5: displayScoreBoardAndReport(); break;
            default: exit(0);
        }
    }
    return 0;
}
