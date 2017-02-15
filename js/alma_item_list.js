(function($) {
  'use strict';

  Drupal.behaviors.alma_item_list = {
    attach: function (context) {
      $('.alma-item-list', context).each(function () {
        var hash = $(this).data('hash');
        var alma_item_list = (Drupal.settings[hash] !== undefined) ? Drupal.settings[hash] : '';
        if (alma_item_list !== '') {
          $.ajax({
            dataType: "json",
            url: Drupal.settings.basePath + 'alma_item_list/' + hash,
            data: {content: alma_item_list},
            success: function (content) {
              $('.pane-alma-item-list', context).find('[data-hash=' + hash + ']').replaceWith(content);
              alma_item_list_add_covers(content);
            }
          });
        }
      });
    }
  };

  var alma_item_list_add_covers = function(content) {
    var cover_data = [];

    // Get items with no covers.
    $.each($('.ting-cover-processing', content), function(info, data) {
      var local_id = $(data).data('local_id');
      cover_data.push({
        local_id: local_id,
        image_style : 'item_list'
      });
    });

    // Make an ajax request for covers.
    if (cover_data.length > 0) {
      //Retrieve covers
      var request = $.ajax({
        url: Drupal.settings.basePath + 'ting/covers',
        type: 'POST',
        data: {
          coverData: cover_data
        },
        dataType: 'json',
        success: ting_cover_insert,
        // Update processing state.
        complete: function(request, status) {
          var processing = $('.ting-cover-processing', content);
          if (status === 'success') {
            processing.addClass('ting-cover-processed');
          }
          processing.removeClass('ting-cover-processing');
        }
      });

      // Associate the request with the context so we can abort the request if
      // the context is detached removed before completion.
      $(content).data('request', request);
    }
  };


  var ting_cover_insert = function(coverData) {
    if(coverData === false){
      return;
    }

    $.each(coverData, function(coverInfo, data) {
      if (data.urls.thumbnail != undefined) {
        var img = '<img src="' + data.urls.thumbnail + '" alt=""/>';
        $('.ting-cover-processing[data-local_id="' + data.local_id + '"] a').html(img);
        $('.ting-cover-processing[data-local_id="' + data.local_id + '"] a').parents('.work-cover').show();
      }
    });
  }

} (jQuery));
