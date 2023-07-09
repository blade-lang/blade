import curl { 
  Option, 
  Info, 
  Curl, 
  CurlList, 
  CurlMime
}
import .message {
  Message
}
import .constants

var _flags_regex = '/\((\\\?([^)]+))?\)/'
var _list_regex = '/^[*] LIST \(\\\?([^)]+)\) "([^"]+)" (.*)$/'

/**
 * The Imap class provides an interface for connecting to an IMAP (Internet Mail Access Protocol) 
 * server and interacting with the server via the IMAP protocol.
 * 
 * This class includes operations for creating, deleting, and renaming mailboxes, checking for new 
 * messages, permanently removing messages, setting and clearing flags searching, and selective 
 * fetching of message attributes, texts, and portions.
 */
class Imap {
  var _curl
  var _base_url

  Imap(options) {
    if options != nil and !is_dict(options)
      die Exception('dictionary expected as argument to constructor')
    if !options options = {}

    
    self._host = options.get('host', 'localhost')
    self._port = options.get('port', 143)
    self._username = options.get('username', nil)
    self._password = options.get('password', nil)
    self._tls = options.get('tls', constants.TLS_TRY)
    self._debug = options.get('debug', false)
    self._verify_peer = options.get('verify_peer', false)
    self._verify_host = options.get('verify_host', false)
    self._proxy = options.get('proxy', nil)
    self._proxy_user = options.get('proxy_username', nil)
    self._proxy_pass = options.get('proxy_password', nil)
    self._verify_proxy_peer = options.get('verify_proxy_peer', self._verify_peer)
    self._verify_proxy_host = options.get('verify_proxy_host', self._verify_host)
    self._timeout = options.get('timeout', 30000)  # in milliseconds (default = 30s)

    self._base_url = '${self._tls == constants.TLS_ALL ? "imaps" : "imap"}://${self._host}:${self._port}'
  }

  _init(url) {
    if !url url = self._base_url
    else url = self._base_url + url

    var curl = self._curl

    if !curl {
      curl = Curl()

      curl.set_option(Option.USE_SSL, self._tls)
      curl.set_option(Option.USERNAME, self._username)
      curl.set_option(Option.PASSWORD, self._password)
      curl.set_option(Option.SSL_VERIFYPEER, self._verify_peer)
      curl.set_option(Option.SSL_VERIFYHOST, self._verify_host)
      curl.set_option(Option.VERBOSE, self._debug)
      curl.set_option(Option.TIMEOUT_MS, self._timeout)
  
      if self._proxy {
        curl.set_option(Option.PROXY, self._proxy)
        if self._proxy_user curl.set_option(Option.PROXYUSERNAME, self._proxy_user)
        if self._proxy_pass curl.set_option(Option.PROXYPASSWORD, self._proxy_pass)
        curl.set_option(Option.PROXY_SSL_VERIFYPEER, self._verify_proxy_peer)
        curl.set_option(Option.PROXY_SSL_VERIFYHOST, self._verify_proxy_host)
      }

      self._curl = curl
    }

    curl.set_option(Option.URL, url)

    return curl
  }

  _to_list(data, type, name) {
    var list = []
    var lines = data.split('\n')

    for line in lines {
      using type {
        when 'list' {
            var match = line.trim().match(_list_regex)
            if match {

              # handle cases where some servers wrap the name in quotations
              match[3] = match[3].trim('"')

              list.append({
                name: match[3],
                separator: match[2],
                flags: match[1].split('/ \\\?/'),
                path: match[3].replace(match[2], '/'),
                is_parent: name ? match[3] == name.trim('/') : false,
              })
            }
        }
        when 'search' {
          line = line.replace('/\\r?\\n/', '').trim().replace('* SEARCH ', '', false)
          if line {
            list.extend(line.split(' '))
          }
        }
      }
    }

    return list
  }

  _examine(data) {
    var lines = data.split('/\\r?\\n/')

    var result = {
      flags: [],
      permanent_flags: [],
      mails: 0,
      recents: 0,
      uid_validity: 0,
      next_uid: 0,
      highest_mod_seq: 0,
      comment: nil,
    }

    for line in lines {
      if line.starts_with('* FLAGS') {
        result.flags = line.match(_flags_regex)[2].split('/ \\\?/')
      } else if line.starts_with('* OK [PERMANENTFLAGS') {
        var matches = line.match(_flags_regex)
        if matches.length() > 1 {
          result.permanent_flags = matches[1].trim('\\').split('/ \\\?/')
        }
        result.comment = line.split('] ')[1]
      } else if line.ends_with(' EXISTS') {
        result.mails = to_number(line[2, line.index_of(' EXISTS')])
      } else if line.ends_with(' RECENT') {
        result.recents = to_number(line[2, line.index_of(' RECENT')])
      } else if line.starts_with('* OK [UIDVALIDITY') {
        result.uid_validity = to_number(line[18, line.index_of(']', 18)])
      } else if line.starts_with('* OK [UIDNEXT') {
        result.next_uid = to_number(line[14, line.index_of(']', 18)])
      } else if line.starts_with('* OK [HIGHESTMODSEQ') {
        result.highest_mod_seq = to_number(line[20, line.index_of(']', 18)])
      }
    }

    return result
  }

  exec(command, path) {
    var curl = self._init(path)
    curl.set_option(Option.CUSTOMREQUEST, command)
    return curl.send().body.to_string()
  }

  get_dirs(path) {
    return self._to_list(self.exec(nil, path), 'list', path)
  }

  get_subscribed_dirs() {
    return self._to_list(self.exec('LSUB "" *'), 'list')
  }

  /**
   * select(name: string)
   * 
   * Instructs the server that the client now wishes to select a particular mailbox or folder 
   * with the name _name_, and any commands that relate to a folder should assume this folder 
   * as the target of that command. For example, an INBOX or a subfolder such as, 
   * "To Do.This Weekend". Once a mailbox is selected, the state of the connection becomes 
   * "Selected".
   * 
   * @see https://www.marshallsoft.com/ImapSearch.htm for more help.
   * @returns dictionary
   */
  select(name) {
    if !name die Exception('name required')
    return self._examine(self.exec('SELECT ${name}'))
  }

  /**
   * examine(name: string)
   * 
   * This function does the exact same thing as `select()`, except that it selects the folder 
   * in read-only mode, meaning that no changes can be effected on the folder.
   * 
   * @returns dictionary
   */
  examine(name) {
    if !name die Exception('name required')
    return self._examine(self.exec('EXAMINE ${name}'))
  }

  /**
   * create(name: string)
   * 
   * Creates a new mailbox or folder with the given name.
   * 
   * @returns list
   */
  create(name) {
    if !name die Exception('name required')
    return self._to_list(self.exec('CREATE ${name}'))
  }

  /**
   * delete(name: string)
   * 
   * Deletes the mailbox or folder with the given name.
   * 
   * @return list
   */
  delete(name) {
    if !name die Exception('name required')
    return self._to_list(self.exec('DELETE ${name}'))
  }

  /**
   * rename(old_name: string, new_name: string)
   * 
   * Renames a mailbox or folder with the name `old_name` to a the name `new_name`.
   * 
   * @return list
   */
  rename(old_name, new_name) {
    if !old_name or !new_name die Exception('old and new name required')
    return self._to_list(self.exec('RENAME ${old_name} ${new_name}'))
  }

  /**
   * subscribe(name: string)
   * 
   * Adds the specified mailbox name to the server's set of "active" or "subscribed" 
   * mailboxes for the current user as returned by `lsub()` and returns `true` if 
   * successful or `false` otherwise.
   * 
   * @return bool
   */
  subscribe(name) {
    if !name die Exception('name required')
    self.exec('SUBSCRIBE ${name}')
    return self._curl.get_info(Info.RESPONSE_CODE) == 250
  }

  /**
   * unsubscribe(name: string)
   * 
   * Removes the specified mailbox name from the server's set of "active" or "subscribed" 
   * mailboxes for the current user as returned by `lsub()` and returns `true` if successful 
   * or `false` otherwise.
   * 
   * @return bool
   */
  unsubscribe(name) {
    if !name die Exception('name required')
    self.exec('UNSUBSCRIBE ${name}')
    return self._curl.get_info(Info.RESPONSE_CODE) == 250
  }

  /**
   * list(name: string, [pattern: string = '%'])
   * 
   * Returns a subset of names from the complete set of all names available to the client. 
   * Zero or more dictionaries are returned, containing the name attributes, hierarchy delimiter, 
   * and name. 
   * 
   * An empty ("" string) _name_ argument indicates that the mailbox name is interpreted 
   * as by SELECT. A non-empty _name_ argument is the name of a mailbox or a level of mailbox 
   * hierarchy, and indicates the context in which the mailbox name is interpreted. 
   * 
   * An empty ("" string) pattern argument is a special request to return the hierarchy delimiter 
   * and the root name of the name given in the reference.
   * 
   * The pattern character `*` is a wildcard, and matches zero or more characters at this position.  
   * The character `%` is similar to `*`, but it does not match a hierarchy delimiter.  If the `%` 
   * wildcard is the last character of a pattern argument, matching levels of hierarchy are also 
   * returned.  If these levels of hierarchy are not also selectable mailboxes, they are returned 
   * with the `\Noselect` pattern attribute.
   * 
   * The special name `INBOX` is included in the output from `list()`, if `INBOX` is supported by 
   * the server for the current user and if the uppercase string "INBOX" matches the interpreted 
   * reference and pattern arguments with wildcards as described above.  The criteria for omitting 
   * INBOX is whether `select('INBOX')` will return failure; it is not relevant whether the user's 
   * real INBOX resides on the server or another.
   * 
   * @return list
   */
  list(name, pattern) {
    if !name die Exception('name required')
    if !pattern and !is_string(pattern) pattern = '%'
    return self._to_list(self.exec('LIST ${name} ${pattern}'))
  }

  /**
   * lsub(name: string, [pattern: string = '%'])
   * 
   * Same as the `list()` function except that it returns a subset of names.
   * 
   * @return list
   */
  lsub(name, pattern) {
    if !name die Exception('name required')
    if !pattern and !is_string(pattern) pattern = '%'
    return self._to_list(self.exec('LSUB ${name} ${pattern}'))
  }

  /**
   * status(name: string, attrs: string)
   * 
   * Requests the status of the indicated mailbox. 
   * 
   * It is important to know that unlike the LIST command, the STATUS command is not 
   * guaranteed to be fast in its response.  Under certain circumstances, it can be 
   * quite slow.
   * 
   * Possible `attrs` values being:
   * 
   * - `MESSAGES`: The number of messages in the mailbox.
   * - `RECENT`: The number of messages with the \Recent flag set.
   * - `UIDNEXT`: The next unique identifier value of the mailbox.
   * - `UIDVALIDITY`: The unique identifier validity value of the mailbox.
   * - `UNSEEN`: The number of messages which do not have the \Seen flag set.
   * 
   * `attrs` values may be separated by space. e.g. `status('INBOX', 'UIDNEXT MESSAGES')`.
   * 
   * @returns bool|string
   */
  status(name, attrs) {
    if !name die Exception('name required')
    if !attrs die Exception('attrs required')
    var result = self.exec('STATUS ${name} (${filter})')
    if self._curl.get_info(Info.RESPONSE_CODE) == 250 {
      var response = {}
      var response_split =  result.split('(')[1][,-1].split(' ')
      iter var i = 0; i < response_split.length(); i++ {
        response[response_split[i]] = response_split[i++]
      }
      return response
    }
    return false
  }

  append(name, message) {
    if !instance_of(message, Message)
      die Exception('instance of Message expected in second argument')

    var examine_result = self.examine(name)
    # var selection_result = self.select(name)
    
    if examine_result {
      var curl = self._init('/' + name)
      var mail = message.build(curl)

      curl.set_option(Option.HTTPHEADER, CurlList(mail.headers))
      curl.set_option(Option.MIMEPOST, mail.mime)

      # append the new email
      curl.send()

      # add response to results
      return curl.get_info(Info.RESPONSE_CODE) == 250
    }

    return false
  }

  check() {
    self.exec('CHECK')
    return self._curl.get_info(Info.RESPONSE_CODE) == 250
  }

  close() {
    self.exec('CLOSE')
    return self._curl.get_info(Info.RESPONSE_CODE) == 250
  }

  expunge(path) {
    self.exec('EXPUNGE', path)
    return self._curl.get_info(Info.RESPONSE_CODE) == 250
  }

  /**
   * Note that query can contain a message sequence set and a number of search 
   * criteria keywords including flags such as ANSWERED, DELETED, DRAFT, FLAGGED, 
   * NEW, RECENT and SEEN. For more information about the search criteria please
   * see RFC-3501 section 6.4.4.
   * 
   * See: https://www.marshallsoft.com/ImapSearch.htm for more
   */
  search(query, folder) {
    if !folder folder = 'INBOX'
    if !query query = 'NEW'

    return self._to_list(self.exec('SEARCH ${query}', '/${folder}'), 'search')
  }

  fetch(id, path) {
    if !id id = 1
    if !path path = 'INBOX'
    return self.exec(nil, '/${path}/;UID=${id}')
  }

  copy(id, destination, path) {
    if !id die Exception('id required')
    if !destination die Exception('destination required')
    self.exec('COPY ${id} ${destination}', path)
    return self._curl.get_info(Info.RESPONSE_CODE) == 250
  }

  /**
   * @note command must be one of `FLAGS`, `+FLAGS`, or `-FLAGS`, optionally with a suffix of `.SILENT`.
   */
  store(id, command, flags) {
    if !id die Exception('id required')
    if !command or !command.match('^[+-]?FLAGS([.]SILENT)?')
      die Exception('invalid command')
    if !flags die Exception('flags required')

    self.exec('STORE ${id} ${command.upper()} ${flags.upper()}', path)
    return self._curl.get_info(Info.RESPONSE_CODE) == 250
  }

  end() {
    if self._curl self._curl.close()
  }

  get_handle() {
    return self._curl
  }
}

def imap(options) {
  if options != nil and !is_dict(options)
    die Exception('dictionary expected as argument to constructor')
  return Imap(options)
}
