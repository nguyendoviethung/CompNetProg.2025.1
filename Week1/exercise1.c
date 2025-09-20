#include <stdio.h>
#include <string.h>
#include <ctype.h>
 
// Lấy kí tự đầu tiên 
const char* getMaterial(char c) {
    switch (c) {
        case '1': return "jets";
        case '2': return "fog";
        case '3': return "foam";
        case '4': return "dry agent";
        default: return NULL;
    }
}
 
// Lấy và xử lí kí tự thứ 2 
void getProperties(char c, int reverse,
                   const char** reactivity,
                   const char** protection,
                   const char** containment) {
    *reactivity = "";
    *protection = "";
    *containment = "";
 
    switch (c) {
        case 'P':
            *reactivity = "can be violently reactive";
            *protection = "full protective clothing must be worn";
            *containment = "may be diluted and washed down the drain";
            break;
        case 'R':
            *reactivity = "can be violently reactive";
            *protection = "full protective clothing must be worn";
            *containment = "";
            break;
        case 'S':
            *reactivity = "can be violently reactive";
            *protection = reverse ?
                "breathing apparatus, protective gloves for fire only"
                : "full protective clothing must be worn";
            *containment = reverse ?
                "may be diluted and washed down the drain"
                : "";
            break;
        case 'T':
            *reactivity = "can be violently reactive";
            *protection = reverse ?
                "breathing apparatus, protective gloves for fire only"
                : "full protective clothing must be worn";
            *containment = reverse ?
                "may be diluted and washed down the drain"
                : "";
            break;
        case 'W':
            *reactivity = "can be violently reactive";
            *protection = "full protective clothing must be worn";
            *containment = "avoid spillages, must be contained";
            break;
        case 'X':
            *reactivity = "can be violently reactive";
            *protection = "full protective clothing must be worn";
            *containment = "";
            break;
        case 'Y':
            *reactivity = "can be violently reactive";
            *protection = reverse ?
                "breathing apparatus, protective gloves for fire only"
                : "full protective clothing must be worn";
            *containment = reverse ?
                "may be diluted and washed down the drain"
                : "";
            break;
        case 'Z':
            *reactivity = "can be violently reactive";
            *protection = reverse ?
                "breathing apparatus, protective gloves for fire only"
                : "full protective clothing must be worn";
            *containment = reverse ?
                "may be diluted and washed down the drain"
                : "";
            break;
    }
}
 
int main() {
    char code[10];
    printf("Enter HAZCHEM code: ");
    scanf("%9s", code);
 
    // Đưa về chữ in hoa
    for (int i = 0; code[i]; i++) {
        code[i] = toupper(code[i]);
    }
 
    int len = strlen(code);
    if (len < 2 || len > 3) {
        printf("Invalid HAZCHEM code!\n");
        return 1;
    }
 
    // Kiểm tra ký tự đầu tiên 
    const char* material = getMaterial(code[0]);
    if (material == NULL) {
        printf("Invalid HAZCHEM code!\n");
        return 1;
    }
 
    // Kiểm tra ký tự thứ hai
    char second = code[1];
    const char* reactivity = NULL;
    const char* protection = NULL;
    const char* containment = NULL;
 
    // Kiểm tra kí tự có màu đảo hay không
    int reverse = 0;
    if (second == 'S' || second == 'T' || second == 'Y' || second == 'Z') {
        char answer[10];
        printf("Is the %c reverse coloured? (yes/no): ", second);
        scanf("%9s", answer);
        if (tolower(answer[0]) == 'y') {
            reverse = 1;
        }
    }
 
    getProperties(second, reverse, &reactivity, &protection, &containment);
    if (reactivity[0] == '\0') {
        printf("Invalid HAZCHEM code!\n");
        return 1;
    }
 
    // Ký tự thứ ba (nếu có)
    int evacuation = 0;
    if (len == 3 && code[2] == 'E') {
        evacuation = 1;
    }
 
    // Xuất kết quả
    printf("\n***Emergency action advice***\n");
    printf("Material:    %s\n", material);
    printf("Reactivity:  %s\n", reactivity);
    printf("Protection:  %s\n", protection);
    if (containment[0] != '\0')
        printf("Containment: %s\n", containment);
    if (evacuation)
        printf("Evacuation:  consider evacuation\n");
    printf("*****************************\n");
 
}