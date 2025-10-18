#include <stddef.h>

// Déclare les fonctions
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
void *memcpy(void *dest, const void *src, size_t n);

int main() {
    char buf[50];
    strcpy(buf, "Fluid + minimal libs!");
    
    // Pour l'instant, pas de printf (on ajoutera plus tard)
    // Juste retourne la longueur comme exit code
    return strlen(buf);
}
