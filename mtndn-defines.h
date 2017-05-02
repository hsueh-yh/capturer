#ifndef DEFINES_H
#define DEFINES_H


#define RESULT_GOOD(res) (res >= RESULT_OK)
#define RESULT_NOT_OK(res) (res < RESULT_OK)
#define RESULT_FAIL(res) (res <= RESULT_ERR)
#define RESULT_NOT_FAIL(res) (res > RESULT_ERR)
#define RESULT_WARNING(res) (res <= RESULT_WARN &&res > RESULT_ERR)

#define RESULT_OK 0
#define RESULT_ERR -100
#define RESULT_WARN -1


#endif // DEFINES_H
