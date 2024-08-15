#include <stdio.h>
#include "cc_array.h"
#include "cc_hashtable.h"
#include "daq.h"

CC_Array *ar;
CC_HashTable *ht;

int add()
{
    char *str = "key1";
    cc_array_add(ar, str);
    cc_array_add(ar, "key2");
    Channel ch;
    channel_init(&ch);
    cc_array_add(ar, &ch);
    
    int a = 1;
    cc_hashtable_add(ht, (void*)"key1", (void*)&a);
    cc_hashtable_add(ht, (void*)"key2", (void*)"value2");

    CC_Array *ar2;
    cc_array_new(&ar2);
    cc_array_add(ar2, "1");
    cc_array_add(ar2, "2");
    cc_array_add(ar2, "3");
    cc_hashtable_add(ht, (void*)"key3", (void*)ar2);
    cc_array_new(&ar2);
    cc_array_add(ar2, "4");
    cc_array_add(ar2, "5");
    cc_array_add(ar2, "6");
    cc_hashtable_add(ht, (void*)"key4", (void*)ar2);

    return 0;
}

int main(int argc, char **argv) {
    while (1)
    {
        cc_array_new(&ar);
        cc_hashtable_new(&ht);
        add();

        char *str1;
        cc_array_get_at(ar, 0, (void*) &str1);
        // printf("%s\n", str1);

        char *str2;
        cc_array_get_at(ar, 1, (void*) &str2);
        // printf("%s\n", str2);

        Channel *ch;
        cc_array_get_at(ar, 2, (void*) &ch);
        channel_free(ch);
        int *b;
        cc_hashtable_get(ht, (void*)"key1", (void*)&b);
        // printf("%d\n", *b);
        char *c;
        cc_hashtable_get(ht, (void*)"key2", (void*)&c);
        // printf("%s\n", c);
        
        CC_Array *ar2;
        cc_hashtable_get(ht, (void*)"key3", (void*)&ar2);
        char *str3;
        cc_array_get_at(ar2, 0, (void*) &str3);
        printf("%s\n", str3);
        printf("addr1:%p\n", ar2);
        cc_array_destroy(ar2);

        cc_hashtable_get(ht, (void*)"key4", (void*)&ar2);
        char *str4;
        cc_array_get_at(ar2, 0, (void*) &str4);
        printf("%s\n", str4);
        printf("addr2:%p\n", ar2);
        cc_array_destroy(ar2);

        cc_array_destroy(ar);
        cc_hashtable_destroy(ht); 
        printf("%s\n", str1);
        printf("ch id: %d\n", ch->id);
        return 0;
        // printf("%s\n", c);
    }
    
    // 创建一个新的数组
    CC_Array *arr;
    cc_array_new(&arr);

    // 添加一些元素到数组
    int num1 = 1, num2 = 2, num3 = 3;
    cc_array_add(arr, &num1);
    cc_array_add(arr, &num2);
    cc_array_add(arr, &num3);

    // 遍历数组并对每个元素进行平方操作
    // CC_ArrayIter iter;
    // cc_array_iter_init(&iter, arr);

    int *num;
    // while (cc_array_iter_next(&iter, &num) != CC_ITER_END) {
    //     printf("Square of %d is %d\n", *num, (*num) * (*num));
    // }
    CC_ARRAY_FOREACH(num, arr, {
        printf("Square of %d\n", *(int*)num);
    });
    // printf("num:%d\n", *num);
    printf("Array size: %d\n", cc_array_size(arr));
    // 销毁数组
    cc_array_destroy(arr);

    return 0;
}