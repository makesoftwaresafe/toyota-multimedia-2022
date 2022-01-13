static void posixErrorDetector(const int err, const char* message) throw (Error)
{
        if (err)
        {
		errno = err;
                perror(message);
                throw Error(String(message) + String(": ") + String(strerror(err)));
        }
}
